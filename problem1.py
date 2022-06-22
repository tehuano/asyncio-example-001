import asyncio
import time
import random
import datetime



# - The program must run for 15 seconds without any of the watchdogs expiring. 
# - You are not able to edit any of the functions you are given.
# - The program should be written in Python (version 3.7) and use the asyncio.
# - Make sure the program prints out each returned value from read_sensor_measurement() and receive_command_from_gui() before petting the watchdog.


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

# wrapper
async def w_pet_task_1_watchdog():
  pet_task_1_watchdog()

async def w_pet_task_2_watchdog():
  pet_task_1_watchdog()

# The first task must call the receive_command_from_gui()
# function and then pet (using pet_task_1_watchdog function)
# a watchdog before it expires. It's watchdog will expire
# after 0.5 seconds.
async def Task01():
  print("Processing Task01")
  L = await asyncio.gather(receive_command_from_gui())
  print(L)
  await asyncio.gather(w_pet_task_1_watchdog())
  

# The second task must call  read_sensor_measurement()
# and then pet (using pet_task_2_watchdog 	function)
# a watchdog before it expires. It's watchdog will expire
# after 2 seconds.
async def Task02():
  print("Processing Task02")
  L = await asyncio.gather(read_sensor_measurement,
    asyncio.to_thread(pet_task_2_watchdog))
  print(L)
  
#loop = asyncio.get_event_loop()
#loop.run_until_complete(myTaskGenerator())
#print("Completed All Tasks")
#loop.close()

async def main():
  loop = asyncio.get_running_loop()
  end_time = loop.time() + 3.0
  
  while True:
    if ((loop.time() + 1.0) >= end_time):
      break
    print(f"00 started at {time.strftime('%X')}")
    t1 = asyncio.create_task(Task01())
    #t2 = asyncio.create_task(Task02)
    print(f"01 started at {time.strftime('%X')}")
    # Wait until both tasks are completed (should take
    # around 2 seconds.)
    await t1
    #await t2
    #await asyncio.sleep(1)

asyncio.run(main())