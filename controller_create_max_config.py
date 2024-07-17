import json
import os
import random
import string
import uuid


########################
number_ports = 32
number_outputs = 32
number_actions = 2
number_tags = 2
max_field_length = 20
######################


def randomChars(maxLength):
    returnValue = ""
    return (returnValue.join(random.choices(string.ascii_lowercase,k=maxLength)))


def createPorts():

    ports = {}

    for i in range(1,number_ports+1):
        ports[i] = {
            "name": randomChars(max_field_length),
            "id":randomChars(8),
            "channels":createChannels()
        }

    return ports

def createChannels():

    channels = {}

    for i in range(1, 4+1):
        if i == 4:
            i = 6
        
        channels[i] = {
            "type":"NORMALLY_CLOSED",
            "enabled": False,
            "offset":99,
            "tags": [],
            "actions":[]
        }

        for j in range(number_tags):
            channels[i]['tags'].append(randomChars(max_field_length))

        for j in range(number_actions):
            action = {
                "change_state": "SHORT",
                "action":"INCREASE",
                "output": 99
            }

            if(i % 2) == 0:
                action["action"] = "DECREASE"

            if(j % 2) == 0:
                action["change_state"] = "LONG"
                action["action"] = action["action"] + "_MAXIMUM"

            channels[i]['actions'].append(action)

    return channels

def createOutputs():
    outputs = {}

    for i in range(1,number_outputs+1):
        outputs[i] = {
            "name": randomChars(max_field_length),
            "id": randomChars(8),
            "area":randomChars(max_field_length),
            "icon": randomChars(64),
            "type": "VARIABLE",
            "enabled": False,
            "tags": [],
            "relay": str(uuid.uuid4())
        }
        
        for j in range(number_tags):
            outputs[i]['tags'].append(randomChars(max_field_length))

    return outputs


def main():


    payload = {}

    outputFileName = os.path.join(os.getcwd(), "controller_max.json")

    payload['name'] = randomChars(max_field_length)
    payload['tags'] = []

    for i in range(number_tags):
        payload['tags'].append(randomChars(max_field_length))


    payload['ports'] = createPorts()
    payload['outputs'] = createOutputs()

    with open(outputFileName, "w") as outfile:
        json.dump(payload, outfile)

    print("Created " + outputFileName)



if __name__ == '__main__':
    main()