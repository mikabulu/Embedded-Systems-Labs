[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/p-H4XMpF)
# Given code for Lab 6

The purpose of this project is to illustrate the use of 
 1. Event flags, which are used to signal between threads
 2. A message queue, which is used to pass messages between threads 

The project uses a serial data link (over USB) to/from a terminal emulator running on the
connected PC / laptop. Coolterm is recommended and a configuration file is included in the project: 
see the lab sheet for more details.

The behaviour is:
- the speed of the green LED and red LED flashing can be turned changed using command entered on a terminal emulator and send over the USB link
- the system is initialised in the RED state 
        
There are two threads:
- t_serial: waits for input from the terminal; sends message to t_flashLED
- t_flashLED: alternates turning the green LED and red LED on. On time changes with the commands sent from the terminal, there are 8 different on times. 
       
The project uses:
 * Three threads
 * Event flags
 * A message queue
 
In addition, the project includes the code for the serial interface. **You are not expected to be able to answer 
questions about this code.** Instead, you should understand the API, which has two functions:
   1. `sendMsg` which writes a message to the terminal
   2. `readLine` which reads a line from the terminal
 
There are further details in the lab sheet and examples in the project code. 
