# SingleSerial
Simple and fast serial library for chips with single hardware serial

speed as FastSerial with tiny size

sizes of sketch DigitalReadSerial from Arduino samples


uses		HardwareSerial		FastSerial	SingleSerial
flash		2636			3848		2300 
ram static	202			109		203
ram full				253


ram full means: 
with SingleSerial & HardwareSerial  RX & TX buffers allocated statically,
where FastSerial uses dynamic memory allocation via malloc()

FastSerial uses 16 bytes TX and 128 RX so full memory usage is 253 bytes
HardwareSerial by default uses 64 bytes TX and 64 RX static buffers
SingleSerial by default uses 16 bytes TX and 128 RX static buffers

see the above table again :)

restrictions: 

* only one port
* buffers 256 bytes max
* no non-blocking write
* buffer sizes must be power of 2

