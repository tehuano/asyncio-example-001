import asyncio
import time
import random
from datetime import datetime

# You have been tasked to write a python program that contains two tasks.
# The first task receives commands from a remote GUI. The second task 
# receives sensor measurements from a piece of hardware. 

# The first task must call the receive_command_from_gui()
# function and then pet (using pet_task_1_watchdog function)
# a watchdog before it expires. It's watchdog will expire
# after 0.5 seconds.

# The second task must call  read_sensor_measurement()
# and then pet (using pet_task_2_watchdog 	function)
# a watchdog before it expires. It's watchdog will expire
# after 2 seconds.

# - The program must run for 15 seconds without any of the watchdogs expiring. 
# - You are not able to edit any of the functions you are given.
# - The program should be written in Python (version 3.7) and use the asyncio.
# - Make sure the program prints out each returned value from 
#   read_sensor_measurement() and receive_command_from_gui() 
#   before petting the watchdog.

def new_pet_task_1_watchdog():
  curr_time = datetime.now()
  formatted_time = curr_time.strftime('%H:%M:%S.%f')[:-3]
  print("Watchdog1: {}".format(formatted_time))
  
def new_pet_task_2_watchdog():
  curr_time = datetime.now()
  formatted_time = curr_time.strftime('%H:%M:%S.%f')[:-3]
  print("Watchdog2: {}".format(formatted_time))
  
def pet_task_1_watchdog():
    print(f"Watchdog1: {time.time()}")
    
def pet_task_2_watchdog():
    print(f"Watchdog2: {time.time()}")
  
async def read_sensor_measurement():
  await asyncio.sleep(1)
  return random.randrange(0, 10)
  
async def receive_command_from_gui():
  await asyncio.sleep(0.1)
  return True

# The first task must call the receive_command_from_gui()
# function and then pet (using pet_task_1_watchdog function)
# a watchdog before it expires. It's watchdog will expire
# after 0.5 seconds.
async def Task01():
  print("\n>>>>> Processing Task01")
  #for x in range(6):
  x = 1
  while True:
    start = time.time()
    L = await asyncio.gather(receive_command_from_gui())
    end = time.time()
    print ("\n# Task01: {}\n  receive_command_from_gui(): {}\n  Elapsed time: {} ms".format(x, L[0],int((end - start) * 1000)))
    pet_task_1_watchdog()
    x = x + 1

# The second task must call  read_sensor_measurement()
# and then pet (using pet_task_2_watchdog 	function)
# a watchdog before it expires. It's watchdog will expire
# after 2 seconds.
async def Task02():
  print("\n>>>>> Processing Task02")
  #for x in range(6):
  x = 1
  while True:
    start = time.time()
    L = await asyncio.gather(read_sensor_measurement())
    end = time.time()
    print ("\n$ Task02: {}\n  read_sensor_measurement(): {}\n  Elapsed time: {} ms".format(x, L[0],int((end - start) * 1000)))
    pet_task_2_watchdog()
    x = x + 1

async def main():
  #await asyncio.gather(Task01(), Task02())
  
  t1 = asyncio.create_task(Task01())
  t2 = asyncio.create_task(Task02())
  
  await t1
  await t2

m = None
def stop():
    m.cancel()


# cancell tasks after 15 secs 

loop = asyncio.get_event_loop()
loop.call_later(15, stop)
m = loop.create_task(main())

try:
    loop.run_until_complete(m)
except asyncio.CancelledError:
    pass
    
#asyncio.run(main())