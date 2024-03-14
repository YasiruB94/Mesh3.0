#include definitions
import binascii
from crc_for_config import *

binary_file = None
start_address_pos = 0x10000
file_name = "example.bin"
total_crc = 0



def open_binary_file_beginning(filename):
    global binary_file
    binary_file = open(filename, 'wb')

def open_binary_file(filename):
    global binary_file
    binary_file = open(filename, 'ab')

def get_file_size():
    global binary_file
    if binary_file:
        #current_size = binary_file.tell() + start_address_pos
        #print("size before writing new info: ", current_size)
        return binary_file.tell()
    else:
        #print("Error: Binary file is not open")
        return -1

def print_amount_of_bytes_at_end():
    print("End byte address: ", hex(get_file_size() + start_address_pos).upper(), "byte size: ", get_file_size())

def append_data_to_binary_file(data):
    global binary_file
    if binary_file:
        #print("Appending data to bin file ... start address: ", hex(get_file_size() + start_address_pos).upper(), "byte size: ", get_file_size())
        binary_file.write(data)
    else:
        print("Error: Binary file is not open")

def calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes():
    global total_crc
    close_binary_file()
    with open(file_name, 'rb') as f:
        f.seek(-254, 2)  # Seek 254 bytes before the end of the file
        last_254_bytes = f.read()  # Read the remaining bytes
        crc_int = cense_crc16_calculate(last_254_bytes)
        total_crc += crc_int
        lsb = crc_int & 0xFF
        # Extract the most significant byte (MSB)
        msb = (crc_int >> 8) & 0xFF
        open_binary_file(file_name)
        print("CRC for 254 bytes starting at address ", hex(get_file_size() + start_address_pos - 254).upper(), " : ", crc_int, "(", hex(crc_int).upper(),")")
        append_data_to_binary_file(bytearray([msb, lsb]))
        return True
    
def calculate_crc_for_the_overall_bin():
    print("starting calculating the total crc")
    close_binary_file()
    with open(file_name, 'rb') as f:
        file = f.read()  # Read the remaining bytes
        crc_int = cense_crc16_calculate(file)
        open_binary_file(file_name)
        print("CRC for the total bin ",  crc_int, "(", hex(crc_int).upper(),")")
        return True
    

def close_binary_file():
    global binary_file
    if binary_file:
        binary_file.close()
        binary_file = None
    else:
        print("Error: Binary file is not open")

def general_configuration():
    print("========== GENERAL CONFIGURATION =============")
    append_data_to_binary_file(bytearray([0x11]))                               #_FLASH_GENERAL__CONFIGURATION_SET
    append_data_to_binary_file(bytearray([1, 4, 7, 0, 0, 0]))                   #_FLASH_SCHEMA__VERSION
    append_data_to_binary_file(bytearray([2, 4, 12, 0, 0, 0]))                  #_FLASH_SCHEMA_MIN_VER__CN
    append_data_to_binary_file(bytearray([0] * 6))                              #_FLASH_SCHEMA_MIN_VER__SW
    append_data_to_binary_file(bytearray([0] * 6))                              #_FLASH_SCHEMA_MIN_VER__DR
    append_data_to_binary_file(bytearray([0] * 6))                              #_FLASH_SCHEMA_MIN_VER__GW
    append_data_to_binary_file(bytearray([0] * 12))                             #[reserved]
    append_data_to_binary_file(bytearray([0]))                                  #_FLASH_GENERAL__DALI_PS_ON
    append_data_to_binary_file(bytearray([8]))                                  #SLOT_COUNT
    append_data_to_binary_file(bytearray([32]))                                 #WIRED_SWITCH_COUNT
    append_data_to_binary_file(bytearray([128]))                                #WIRELESS_SWITCH_COUNT
    append_data_to_binary_file(bytearray([77,0]))                               #CABINET_NUMBER
    append_data_to_binary_file(bytearray([0] * 205))                            #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--
    append_data_to_binary_file(bytearray([0] * 3840))                           #[reserved]
    append_data_to_binary_file(bytearray([0] * 61440))                          #[reserved]
    print_amount_of_bytes_at_end()
    print("======= END OF GENERAL CONFIGURATION =================")

def channelConfig(number):

    max_current = number #temp
    if(max_current == 1800):
        append_data_to_binary_file(bytearray([0x07, 0x08]))                  #MAX current
    else:
        append_data_to_binary_file(bytearray([0, max_current]))                  #MAX current
    append_data_to_binary_file(bytearray([0, 0]))                                #CCT starting current
    append_data_to_binary_file(bytearray([0] * 4))                               #[reserved] (setting 2 ?)
    append_data_to_binary_file(bytearray([128]))                                 #active(1b), bonded(1b), pf restore(1b), fixture type(3b), [reserved] (2b)
    append_data_to_binary_file(bytearray([0]))                                   #next_bonded_ch_slot(3b), next_bonded_ch_ch(4b), responds_to_dali(1b)
    append_data_to_binary_file(bytearray([0]))                                   #dali_address
    append_data_to_binary_file(bytearray([0]))                                   #[reserved]
    append_data_to_binary_file(bytearray([0] * 2))                               #responds_to_dmx(1b), [reserved](6b), dmx_address(9b)
    append_data_to_binary_file(bytearray([0]))                                   #[reserved](7b), expose_channel_to_gateway(1b)
    append_data_to_binary_file(bytearray([0]))                                   #expose_channel_as_group
    append_data_to_binary_file(bytearray([0] * 47))                              #[reserved]

def groupMembership(number):
    append_data_to_binary_file(bytearray([0] * 32))                              #[reserved] TEMPORARY

def sceneMembership(channel_num, number):
    append_data_to_binary_file(bytearray([0] * 224))                             #[reserved] TEMPORARY

    

def driver_slot_configuration(number):
    print("======= DRIVER SLOT [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([1]))                                  #reserved + config_status
    append_data_to_binary_file(bytearray([4]))                                  #driver_count
    append_data_to_binary_file(bytearray([0] * 252))                            #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--
    if number == 1:
        channelConfig(1800)
        channelConfig(1800)
        channelConfig(1800)
        channelConfig(1800)
    else:
        channelConfig(180)
        channelConfig(180)
        channelConfig(180)
        channelConfig(180)


    append_data_to_binary_file(bytearray([0] * 2))                              #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    groupMembership(0)
    groupMembership(1)
    groupMembership(2)
    groupMembership(3)
    append_data_to_binary_file(bytearray([0] * 126))                            #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    append_data_to_binary_file(bytearray([0] * 254))                            #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [0-31] ch0
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [32-63]ch0
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [0-31] ch1
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [32-63]ch1
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [0-31] ch2
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [32-63]ch2
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [0-31] ch3
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    sceneMembership(0, 0)                                                       #sceneMembership [32-63]ch3
    append_data_to_binary_file(bytearray([0] * 30))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    append_data_to_binary_file(bytearray([0] * 31))                             #Channel 0 display name
    append_data_to_binary_file(bytearray([0] * 31))                             #Channel 1 display name
    append_data_to_binary_file(bytearray([0] * 31))                             #Channel 2 display name
    append_data_to_binary_file(bytearray([0] * 31))                             #Channel 3 display name
    append_data_to_binary_file(bytearray([0] * 130))                            #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    append_data_to_binary_file(bytearray([0] * 768))                            #[reserved]
    print_amount_of_bytes_at_end()
    print("======= END OF DRIVER SLOT [",number,"] CONFIGURATION =======")

def SW_Target(slot, channel):
    append_data_to_binary_file(bytearray([1]))                                  #target_type
    append_data_to_binary_file(bytearray([0,77]))                               #[reserved](7b) + cabinet_address(9b)
    append_data_to_binary_file(bytearray([slot]))                               #slot
    append_data_to_binary_file(bytearray([channel]))                            #channel 
    append_data_to_binary_file(bytearray([0]))                                  #group
    append_data_to_binary_file(bytearray([0]))                                  #scene
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

def SW_Target_wireless(slot, channel):
    append_data_to_binary_file(bytearray([15]))                                 #target_type
    append_data_to_binary_file(bytearray([0,0]))                                #[reserved](7b) + cabinet_address(9b)
    append_data_to_binary_file(bytearray([slot]))                               #slot
    append_data_to_binary_file(bytearray([channel]))                            #channel 
    append_data_to_binary_file(bytearray([0]))                                  #group
    append_data_to_binary_file(bytearray([0]))                                  #scene
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

def SW_Command(trigger, action,p1, p2, p3,p4):
    append_data_to_binary_file(bytearray([trigger]))                             #trigger
    append_data_to_binary_file(bytearray([action]))                              #action
    append_data_to_binary_file(bytearray([p1,p2,p3,p4]))                         #parameters
    append_data_to_binary_file(bytearray([0] * 10))                              #[reserved]



def wired_switch_configuration(number,channel, slot):
    print("======= WIRED SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([0]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0] * 2))                              #parameters
    append_data_to_binary_file(bytearray([0] * 4))                              #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    SW_Target(slot, channel)
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]
    SW_Command(7, 1, 0, 0, 1, 144)
    SW_Command(8, 3, 0, 0, 1, 144)
    SW_Command(0, 0, 0, 0, 0, 0)
    SW_Command(0, 0, 0, 0, 0, 0)
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRED SWITCH [",number,"] CONFIGURATION =======")

def wired_switch_configuration_08(number,channel, slot):
    print("======= WIRED SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([1]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0, 201]))                              #parameters
    append_data_to_binary_file(bytearray([0] * 4))                              #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    SW_Target(slot, channel)
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]
    SW_Command(1, 14, 0, 255, 225, 144)
    SW_Command(0, 0, 0, 0, 0, 0)
    SW_Command(0, 0, 0, 0, 0, 0)
    SW_Command(0, 0, 0, 0, 0, 0)
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRED SWITCH [",number,"] CONFIGURATION =======")


def wireless_switch_configuration_00(number,channel, slot):
    print("======= WIRELESS SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([1]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0, 201]))                             #parameters
    append_data_to_binary_file(bytearray([40, 71, 40, 4]))                      #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    SW_Target_wireless(slot, channel)
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]
    SW_Command(1, 1, 0, 0, 1, 144)
    SW_Command(4, 12, 0, 2, 128, 0)
    SW_Command(5, 12, 0, 2, 128, 0)
    SW_Command(0, 0, 0, 0, 0, 0)
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRELESS SWITCH [",number,"] CONFIGURATION =======")

def wireless_switch_configuration_01(number,channel, slot):
    print("======= WIRELESS SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([1]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0, 201]))                             #parameters
    append_data_to_binary_file(bytearray([40, 71, 40, 4]))                      #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    SW_Target_wireless(slot, channel)
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]
    SW_Command(1, 3, 0, 0, 1, 144)
    SW_Command(4, 13, 0, 2, 128, 0)
    SW_Command(5, 13, 0, 2, 128, 0)
    SW_Command(0, 0, 0, 0, 0, 0)
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRELESS SWITCH [",number,"] CONFIGURATION =======")

def wireless_switch_configuration_02(number,channel, slot):
    print("======= WIRELESS SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([1]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0, 201]))                             #parameters
    append_data_to_binary_file(bytearray([0, 0, 0, 0]))                      #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 0
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]

    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 0
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 1
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 2
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 3
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]

    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRELESS SWITCH [",number,"] CONFIGURATION =======")

def wireless_switch_configuration_empty(number):
    print("======= WIRELESS SWITCH [",number,"] CONFIGURATION ==============")
    append_data_to_binary_file(bytearray([0]))                                  #reserved + type
    append_data_to_binary_file(bytearray([0, 0]))                               #parameters
    append_data_to_binary_file(bytearray([0, 0, 0, 0]))                         #ID (wireless)
    append_data_to_binary_file(bytearray([0] * 11))                             #[reserved]

    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 0
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 1
    append_data_to_binary_file(bytearray([0] * 18))                             #SW target 2
    append_data_to_binary_file(bytearray([0] * 54))                             #[reserved]

    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 0
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 1
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 2
    append_data_to_binary_file(bytearray([0] * 16))                             #SW command 3
    append_data_to_binary_file(bytearray([0] * 64))                             #[reserved]

    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--


    print("======= END OF WIRELESS SWITCH [",number,"] CONFIGURATION =======")

def wireless_sensor_configuration_empty(number):
    print("======= WIRELESS SENSOR [",number, number+1,"] CONFIGURATION ==============")

    append_data_to_binary_file(bytearray([0] * 127))                             #[reserved]
    append_data_to_binary_file(bytearray([0] * 127))                             #[reserved]
    calculate_crc_for_the_last_254_bytes_and_add_it_to_the_last_two_bytes()     #--PAGE CRC--

    print("======= END OF WIRELESS SENSOR [",number,number+1,"] CONFIGURATION =======")


if __name__ == "__main__":
    print("creating binary file")
    open_binary_file_beginning(file_name)

    general_configuration()
    for i in range(0,8):
        driver_slot_configuration(i)

    append_data_to_binary_file(bytearray([0] * 32768))                            #[reserved]
    print_amount_of_bytes_at_end()
    slot = -1
    channel = -1
    for i in range(0,8):
        if i % 4 == 0:
            slot+=1
            channel = -1
        channel +=1
        print("slot: ", slot, "channel: ",channel)
        wired_switch_configuration(i, channel, slot)
    
    wired_switch_configuration_08(8, 0, 2)
    wired_switch_configuration_08(9, 1, 2)
    wired_switch_configuration_08(10, 2, 2)
    wired_switch_configuration_08(11, 3, 2)
    slot+=1
    for i in range(12,32):
        if i % 4 == 0:
            slot+=1
            channel = -1
        channel +=1
        print("slot: ", slot, "channel: ",channel)
        wired_switch_configuration(i, channel, slot)
    append_data_to_binary_file(bytearray([0] * 4096))                            #[reserved]
    print_amount_of_bytes_at_end()

    wireless_switch_configuration_00(0,0,0)
    wireless_switch_configuration_01(1,0,0)
    wireless_switch_configuration_02(2,0,0)
    wireless_switch_configuration_02(3,0,0)

    for i in range(4,128):
        wireless_switch_configuration_empty(i)

    # wireless sensor configuration
    for i in range(0, 32, 2):
        wireless_sensor_configuration_empty(i)
    
    #append_data_to_binary_file(bytearray([0] * 16384))                            #[reserved]
    #append_data_to_binary_file(bytearray([0] * 327680))                           #[reserved]
    print_amount_of_bytes_at_end()
    print("DONE. Total CRC = ", total_crc , "(",hex(total_crc), ")")
    #calculate_crc_for_the_overall_bin()

