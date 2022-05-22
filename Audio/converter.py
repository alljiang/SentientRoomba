
import os
from pydub import AudioSegment

input_path = "Audio/input/"
input_path_sfw_hq = input_path + "sfw-hq/"
input_path_sfw_lq = input_path + "sfw-lq/"
input_path_nsfw_hq = input_path + "nsfw-hq/"
input_path_nsfw_lq = input_path + "nsfw-lq/"

output_path = "Audio/output/"

bit_rate = "56k"
sample_rate = "44100"
bit_depth = "16"

file_list = []

directory_file_list = []
for file in os.listdir(input_path_sfw_hq):
    if file.endswith(".mp3") or file.endswith(".wav") or file.endswith(".flac") or file.endswith(".ogg") or file.endswith(".aac") or file.endswith(".m4a"):
        directory_file_list.append(file)
file_list.append(directory_file_list)

directory_file_list = []
for file in os.listdir(input_path_sfw_lq):
    if file.endswith(".mp3") or file.endswith(".wav") or file.endswith(".flac") or file.endswith(".ogg") or file.endswith(".aac") or file.endswith(".m4a"):
        directory_file_list.append(file)
file_list.append(directory_file_list)

directory_file_list = []
for file in os.listdir(input_path_nsfw_hq):
    if file.endswith(".mp3") or file.endswith(".wav") or file.endswith(".flac") or file.endswith(".ogg") or file.endswith(".aac") or file.endswith(".m4a"):
        directory_file_list.append(file)
file_list.append(directory_file_list)

directory_file_list = []
for file in os.listdir(input_path_nsfw_lq):
    if file.endswith(".mp3") or file.endswith(".wav") or file.endswith(".flac") or file.endswith(".ogg") or file.endswith(".aac") or file.endswith(".m4a"):
        directory_file_list.append(file)
file_list.append(directory_file_list)

print()

# read files, convert to .mp3, and save to output directory
for i in range(len(file_list)):
    counter = 0
    directory = file_list[i]
    
    input_path_type = ""
    macro = ""
    header_num = 0
    if i == 0:
        input_path_type = input_path_sfw_hq
        header_num = 1
        macro = "SFW_HQ_COUNT"
    elif i == 1:
        input_path_type = input_path_sfw_lq
        header_num = 2
        macro = "SFW_LQ_COUNT"
    elif i == 2:
        input_path_type = input_path_nsfw_hq
        header_num = 3
        macro = "NSFW_HQ_COUNT"
    elif i == 3:
        input_path_type = input_path_nsfw_lq
        header_num = 4
        macro = "NSFW_LQ_COUNT"

    for file in directory:
        audio = AudioSegment.from_file(input_path_type + file)
        
        new_file_name = str(header_num) + str(counter).zfill(4) + ".mp3"

        audio = audio.set_frame_rate(int(sample_rate))
        audio = audio.set_sample_width(int(bit_depth) / 8)
        audio = audio.set_channels(1)

        # convert to mono
        audio.export(output_path + new_file_name, format="mp3", bitrate=bit_rate)

        counter += 1

    print("#define " + macro + " " + str(counter))

print()