import os
import pathlib 

def get_documented_command_names():
    docs_dir = pathlib.Path(__file__).parent.parent / "commands"
    all_commands = []
    for file_name in os.listdir(docs_dir):
        file_path = docs_dir / file_name
        commands = [file_name.split('.')[0]]
        with open(file_path, 'r') as infile:
            for line in infile:
                if line.startswith('for_commands:'):
                    commands = line.split(':')[1].strip().split(' ')
        all_commands.extend(commands)
    return all_commands

def get_all_commands_names():
    commands_file_dir = pathlib.Path(__file__).parent.parent.parent / "pdf_viewer" / "input.cpp"
    all_commands = []
    with open(commands_file_dir, 'r') as infile:
        for line in infile:
            index = line.find('static inline const std::string cname')
            if index >=0 :
                all_commands.append(line.split('"')[1])
    return all_commands

if __name__ == '__main__':
    documented_commands = set(get_documented_command_names())
    all_commands = set(get_all_commands_names())
    undocumented_commands = all_commands - documented_commands
    for com in undocumented_commands:
        print(com)
    print('#all commands:', len(all_commands))
    print('#documented commands:', len(documented_commands))
    print('#undocumented commands:', len(undocumented_commands))