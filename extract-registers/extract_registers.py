import json
import os
import sys
import traceback

import mca3k_data

def extract_registers(settings):
    for k in settings:
        loaded = settings[k]['fields']
        obj = eval(f'mca3k_data.{k}()')
        obj.fields = loaded
        try:
            obj.fields_2_user()
            obj.fields_2_registers()
            print(f'Computed registers for {k}:')
            print(obj.registers)
        except KeyError as e:
            print(
                "*" * 80,
                "There's a missing key from the loaded-in settings file.",
                "This is probably a 'self-clearing' value that only changes at runtime but didn't get saved to the JSON.",
                f"The missing key was in the command: {k}.",
                "The fix is to add a reasonable value to the JSON file. For instance, run_action in arm_ctrl can be set to 0.0.",
                "",
                "Here is the traceback for more information:", sep=os.linesep)
            traceback.print_exc()
            print("*" * 80)


def main():
    if len(sys.argv) != 2:
        print('Usage: python extract_registers.py [settings file name]')
        exit(1)

    with open(sys.argv[1], 'r') as f:
        settings = json.loads(f.read())

    extract_registers(settings)


if __name__ == '__main__': main()
