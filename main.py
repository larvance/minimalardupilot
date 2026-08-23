#!/usr/bin/env python3
import os
import subprocess
import time
from pathlib import Path

BASE_DIR = Path(__file__).parent.resolve()
REPO_DIR = BASE_DIR / "repo"

UPSTREAM_URL = "https://github.com/ArduPilot/ardupilot.git"
PREFIXES = ("Tracker-", "Sub-", "Rover-", "Plane-", "Copter-", "AP_Periph-")


def get_target_url():
    res = subprocess.run("git config --get remote.origin.url", cwd=BASE_DIR, shell=True, text=True, capture_output=True)
    if res.returncode != 0 or not res.stdout.strip():
        raise RuntimeError("Could not determine remote.origin.url from the base repository.")
    return res.stdout.strip()


def run_cmd(cmd, cwd=REPO_DIR, env=None, check=True):
    res = subprocess.run(cmd, cwd=cwd, env=env, shell=True, text=True, capture_output=True)
    if check and res.returncode != 0:
        print(f"Error running: {cmd}\n{res.stderr}")
        raise RuntimeError(res.stderr)
    return res.stdout.strip()


def resolve_git_url(base, rel):
    rel = rel.strip()

    if rel.startswith("git://github.com/"):
        rel = "https://github.com/" + rel[len("git://github.com/"):]
    elif rel.startswith("git://"):
        rel = "https://" + rel[len("git://"):]
    elif rel.startswith("git@github.com:"):
        rel = "https://github.com/" + rel[len("git@github.com:"):]
    elif rel.startswith("http://"):
        rel = "https://" + rel[len("http://"):]

    if any(rel.startswith(proto) for proto in ("https://", "http://", "git://", "git@")):
        url = rel
    else:
        base = base.rstrip("/")
        if base.endswith(".git"):
            base = base[:-4]
        parts = base.split("/")
        for p in rel.split("/"):
            if p == "..":
                if len(parts) > 3:
                    parts.pop()
            elif p != "." and p != "":
                parts.append(p)
        url = "/".join(parts)

    if not url.endswith(".git"):
        url += ".git"

    return url


def get_remote_refs(url, kind="tags"):
    out = run_cmd(f"git ls-remote --{kind} {url}")
    refs = set()
    for line in out.splitlines():
        if not line: continue
        sha, ref = line.split()
        clean_ref = ref.replace(f"refs/{kind}/", "").replace("^{}", "")
        refs.add(clean_ref)
    return refs


def get_submodules_for_commit(ref_or_sha, parent_url):
    tree_out = run_cmd(f"git ls-tree -r {ref_or_sha}")
    gitlinks = {}
    for line in tree_out.splitlines():
        parts = line.split(maxsplit=3)
        if len(parts) == 4 and parts[0] == "160000":
            gitlinks[parts[3]] = parts[2]

    if not gitlinks:
        return []

    try:
        content = run_cmd(f"git show {ref_or_sha}:.gitmodules")
    except RuntimeError:
        return []

    tmp_file = REPO_DIR / ".gitmodules.tmp"
    tmp_file.write_text(content)

    path_to_name = {}
    for line in run_cmd(f"git config --file {tmp_file.name} --get-regexp " + r'"\.path$"', check=False).splitlines():
        if line:
            key, val = line.split(maxsplit=1)
            path_to_name[val] = key[10:-5]

    name_to_url = {}
    for line in run_cmd(f"git config --file {tmp_file.name} --get-regexp " + r'"\.url$"', check=False).splitlines():
        if line:
            key, val = line.split(maxsplit=1)
            name_to_url[key[10:-4]] = val

    tmp_file.unlink(missing_ok=True)

    submodules = []
    for path, sha in gitlinks.items():
        name = path_to_name.get(path)
        if name and name in name_to_url:
            raw_url = name_to_url[name]
            url = resolve_git_url(parent_url, raw_url)
            submodules.append({"path": path, "url": url, "sha": sha})

    return submodules


def inline_submodules_recursive(env, ref_or_sha, parent_url, current_prefix=""):
    subs = get_submodules_for_commit(ref_or_sha, parent_url)
    for sub in subs:
        rel_path = sub["path"]
        sub_sha = sub["sha"]
        sub_url = sub["url"]
        full_path = f"{current_prefix}/{rel_path}" if current_prefix else rel_path

        print(f"  Inlining submodule {full_path} ({sub_sha[:7]})...")
        try:
            run_cmd(f"git fetch {sub_url} {sub_sha}")
        except RuntimeError:
            print(f"  Exact fetch failed, falling back to full fetch for {sub_url}")
            run_cmd(f"git fetch {sub_url}")

        run_cmd(f"git rm -r --cached --ignore-unmatch {full_path}", env=env, check=False)
        run_cmd(f"git read-tree --prefix={full_path}/ {sub_sha}", env=env)

        inline_submodules_recursive(env, sub_sha, sub_url, current_prefix=full_path)


def create_and_push_orphan(tag):
    env = os.environ.copy()
    idx_file = REPO_DIR / "temp_index"
    env["GIT_INDEX_FILE"] = str(idx_file)

    idx_file.unlink(missing_ok=True)
    ref_or_sha = f"refs/tags/{tag}"
    run_cmd(f"git read-tree {ref_or_sha}", env=env)

    inline_submodules_recursive(env, ref_or_sha, UPSTREAM_URL)

    for f in run_cmd("git ls-files \"*.gitmodules\"", env=env, check=False).splitlines():
        if f.strip():
            run_cmd(f"git rm --cached {f.strip()}", env=env, check=False)

    run_cmd("git rm -r --cached --ignore-unmatch .github/workflows", env=env, check=False)

    tree = run_cmd("git write-tree", env=env)
    idx_file.unlink(missing_ok=True)

    env["GIT_AUTHOR_NAME"] = run_cmd(f"git log -1 --format=\"%an\" {ref_or_sha}")
    env["GIT_AUTHOR_EMAIL"] = run_cmd(f"git log -1 --format=\"%ae\" {ref_or_sha}")
    env["GIT_AUTHOR_DATE"] = run_cmd(f"git log -1 --format=\"%ad\" {ref_or_sha}")
    env["GIT_COMMITTER_NAME"] = run_cmd(f"git log -1 --format=\"%cn\" {ref_or_sha}")
    env["GIT_COMMITTER_EMAIL"] = run_cmd(f"git log -1 --format=\"%ce\" {ref_or_sha}")
    env["GIT_COMMITTER_DATE"] = run_cmd(f"git log -1 --format=\"%cd\" {ref_or_sha}")

    msg = run_cmd(f"git log -1 --format=\"%B\" {ref_or_sha}")
    msg_file = REPO_DIR / "commit_msg.tmp"
    msg_file.write_text(msg)

    new_commit = run_cmd(f"git commit-tree {tree} < commit_msg.tmp", env=env)
    msg_file.unlink(missing_ok=True)

    run_cmd(f"git update-ref refs/heads/{tag} {new_commit}")
    print(f"  Pushing {tag} (with inlined submodules)...")
    run_cmd(f"git push target refs/heads/{tag}:{tag} --force")


def setup_repo():
    target_url = get_target_url()
    if not REPO_DIR.exists():
        REPO_DIR.mkdir(parents=True)
        run_cmd("git init")
        run_cmd(f"git remote add upstream {UPSTREAM_URL}")
        run_cmd(f"git remote add target {target_url}")
    else:
        run_cmd(f"git remote set-url target {target_url}")

    run_cmd("git config url.\"https://github.com/\".insteadOf git://github.com/", check=False)


def main():
    print("Starting ArduPilot update daemon...")
    setup_repo()

    while True:
        try:
            current_time = time.strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{current_time}] Checking for updates...")
            run_cmd("git fetch upstream --tags")

            target_url = get_target_url()
            existing_branches = get_remote_refs(target_url, "heads")
            upstream_tags = get_remote_refs(UPSTREAM_URL, "tags")

            valid_tags = [t for t in upstream_tags if any(t.startswith(p) for p in PREFIXES)]

            for tag in valid_tags:
                if tag not in existing_branches:
                    print(f"\nNew tag detected: {tag}")
                    run_cmd(f"git fetch upstream refs/tags/{tag}:refs/tags/{tag} --depth=1")
                    create_and_push_orphan(tag)

        except Exception as e:
            print(f"An error occurred during the loop: {e}")

        print("Sleeping for 1 hour...")
        time.sleep(3600)


if __name__ == "__main__":
    main()
