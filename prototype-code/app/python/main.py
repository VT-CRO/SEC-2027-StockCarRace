import time

from arduino.app_utils import App, Bridge


# def is_ready() -> bool:
#     return True

print("Python app started. Use arduino-app-cli monitor to view logs from MCU.")
# Bridge.provide("logln", lambda msg: print("MCU Log:", msg, flush=True))
# Bridge.provide("log", lambda msg: print("MCU Log:", msg, end="", flush=True))
# Bridge.provide("is_ready", is_ready)

def loop():
    """This function is called repeatedly by the App framework."""
    # You can replace this with any code you want your App to run repeatedly.
    time.sleep(10)


# See: https://docs.arduino.cc/software/app-lab/tutorials/getting-started/#app-run
App.run(user_loop=loop)
