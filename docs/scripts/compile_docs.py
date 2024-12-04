#%%
from sioyek import sioyek
import os
import pathlib
import time
import shutil
import pypandoc
import re
import argparse
import sys
import tqdm
import json
#%%

config_names_file = pathlib.Path(__file__).parent / "config_names.txt"
video_base_path = pathlib.Path(__file__).parent.parent / "videos"
files_base_path = pathlib.Path(__file__).parent.parent / "files"
docs_base_path = pathlib.Path(__file__).parent.parent
tutorial_base_path = pathlib.Path(__file__).parent.parent.parent / "tutorial"
tutorial_file_path = pathlib.Path(__file__).parent.parent.parent / "tutorial" / "tut.pdf"
tutorial_tex_path = pathlib.Path(__file__).parent.parent.parent / "tutorial" / "tut.tex"
markdown_file_path = pathlib.Path(r'C:\sioyek\sioyek-new\sioyek\docs\commands\goto_definition.md')

def create_arg_parser():
    parser = argparse.ArgumentParser(
        prog='sioyek_database_utils',
        )

    parser.add_argument('--output-dir', type=str, help='Path to the local database file (local.db)', default='.')
    return parser

def get_video_file_path_for_markdown_file(markdown_file_path):
    return video_base_path / (markdown_file_path.relative_to(docs_base_path).with_suffix('.mp4'))

def get_attribute_list(attribute_name, file_content):
    attribute_index = file_content.find(attribute_name)
    if attribute_index >= 0:
        attribute_line = file_content[attribute_index + len(attribute_name):file_content.find('\n', attribute_index)].strip()
        if len(attribute_line) > 0:
            return attribute_line.split(' ')
    return []

def resolve_configs_and_commands(
        markdown,
        commands_file_name,
        configs_file_name,
        commands_title_map,
        configs_title_map):
    config_regex = re.compile("@config\([a-z0-9_]*\)")
    command_regex = re.compile("@command\([a-z0-9_]*\)")
    concept_regex = re.compile("@concept\([a-z0-9_]*\)")

    # replace all instances of these regexes with the contents
    # of parentheisized part
    def translate_command(match):
        command_name = match.group()[9:-1]
        return f'[`{command_name}`]({commands_file_name}#{commands_title_map[command_name]})'
    
    def translate_config(match):
        config_name = match.group()[8:-1]
        return f'[`{config_name}`]({configs_file_name}#{configs_title_map[config_name]})'
    
    # markdown = config_regex.sub(lambda x: '`' + x.group()[8:-1] + '`', markdown)
    # markdown = command_regex.sub(lambda x: '`' + x.group()[9:-1] + '`', markdown)

    try:
        markdown = config_regex.sub(translate_config, markdown)
        markdown = command_regex.sub(translate_command, markdown)
        markdown = concept_regex.sub(lambda x: '`' + x.group()[9:-1] + '`', markdown)
        return markdown
    except Exception as e:
        print('there was an error processing this markdown:')
        print(markdown)
        raise e

def get_documentation_maps(commands_file_name, configs_file_name):
    all_config_names = []
    for line in open(config_names_file, "r", encoding='utf8'):
        all_config_names.append(line.strip()[1:-1])
    
    # print(all_config_names)

    command_file_paths = []
    config_file_paths = []

    command_name_to_title_map = dict()
    config_name_to_title_map = dict()

    command_title_to_documentation_map = dict()
    config_title_to_documentation_map = dict()
    command_related_commands_map = dict()
    command_related_configs_map = dict()
    config_related_commands_map = dict()
    config_related_configs_map = dict()
    command_name_to_file_name_map = dict()
    config_name_to_file_name_map = dict()

    for file in os.listdir(docs_base_path / 'commands'):
        markdown_file_path = docs_base_path / 'commands' / file
        command_file_paths.append(markdown_file_path)

    for file in os.listdir(docs_base_path / 'configs'):
        markdown_file_path = docs_base_path / 'configs' / file
        config_file_paths.append(markdown_file_path)

    all_files = command_file_paths + config_file_paths

    for markdown_file_path in all_files:

        with open(markdown_file_path, 'r', encoding='utf8') as infile:
            file_content = infile.read()

        document_type = markdown_file_path.parent.name
        for_commands = get_attribute_list('for_commands:', file_content)
        for_configs = get_attribute_list('for_configs:', file_content)

        if for_commands == []:
            for_commands = [markdown_file_path.stem]
        if for_configs == []:
            for_configs = [markdown_file_path.stem]

        if document_type == 'commands':
            title = '-'.join(for_commands)
            if markdown_file_path.stem not in for_commands:
                for_commands.append(markdown_file_path.stem)
            for command in for_commands:
                command_name_to_title_map[command] = title
                command_name_to_file_name_map[command] = markdown_file_path.stem
            command_name_to_file_name_map[title] = markdown_file_path.stem

        if document_type == 'configs':
            title = '-'.join(for_configs)
            if markdown_file_path.stem not in for_configs:
                for_configs.append(markdown_file_path.stem)
            for config in for_configs:
                config_name_to_title_map[config] = title
                config_name_to_file_name_map[config] = markdown_file_path.stem

                if ('*' in config) or ('[' in config): # config name is a pattern
                    config = config.replace('*', '.*')
                    config_pattern = re.compile(config)
                    for full_config_name in all_config_names:
                        if config_pattern.match(full_config_name):
                            config_name_to_file_name_map[full_config_name] = markdown_file_path.stem

            config_name_to_file_name_map[title] = markdown_file_path.stem
        

    for markdown_file_path in tqdm.tqdm(all_files):

        with open(markdown_file_path, 'r', encoding='utf8') as infile:
            file_content = infile.read()

        document_type = markdown_file_path.parent.name

        for_commands = get_attribute_list('for_commands:', file_content)
        related_commands = get_attribute_list('related_commands:', file_content)
        for_configs = get_attribute_list('for_configs:', file_content)
        related_configs = get_attribute_list('related_configs:', file_content)
        should_hide = get_attribute_list('hide:', file_content) == ['true']
        requires_pro = get_attribute_list('requires_pro:', file_content) == ['true']

        if should_hide:
            continue

        doc_body_begin_index = file_content.find("doc_body:")
        if doc_body_begin_index >= 0:
            markdown_content = file_content[doc_body_begin_index + len("doc_body:"):].strip()
            markdown_content = resolve_configs_and_commands(
                markdown_content,
                commands_file_name,
                configs_file_name,
                command_name_to_title_map,
                config_name_to_title_map
                )

            if document_type == 'commands':
                if len(for_commands) == 0:
                    for_commands = [markdown_file_path.stem]

                # for command in for_commands:
                #     command_related_commands_map[command] = related_commands
                #     command_related_configs_map[command] = related_configs

                current_doc = ''
                link_title = '-'.join(for_commands)
                title = '# ' + ', '.join(for_commands) + '\n'
                current_doc += title
                # commands_documentation += '-' * len(title) + '\n\n'
                current_doc += markdown_content

                command_related_commands_map[link_title] = related_commands
                command_related_configs_map[link_title] = related_configs

                command_title_to_documentation_map[link_title] = current_doc

            elif document_type == 'configs':
                if len(for_configs) == 0:
                    for_configs = [markdown_file_path.stem]

                # for config in for_configs:
                #     config_related_commands_map[config] = related_commands
                #     config_related_configs_map[config] = related_configs

                current_doc = ''
                link_title = '-'.join(for_configs)
                title = '# ' + ', '.join(for_configs) + '\n'
                current_doc += title
                current_doc += markdown_content
                config_related_commands_map[link_title] = related_commands
                config_related_configs_map[link_title] = related_configs
                config_title_to_documentation_map[link_title] = current_doc
    return {
        'command_name_to_title_map': command_name_to_title_map,
        'config_name_to_title_map': config_name_to_title_map,
        'command_title_to_documentation_map': command_title_to_documentation_map,
        'config_title_to_documentation_map': config_title_to_documentation_map,
        'command_related_commands_map': command_related_commands_map,
        'command_related_configs_map': command_related_configs_map,
        'config_related_commands_map': config_related_commands_map,
        'config_related_configs_map': config_related_configs_map,
        'command_name_to_file_name_map': command_name_to_file_name_map,
        'config_name_to_file_name_map': config_name_to_file_name_map,
    }
        

def generate_restructured_text_documentation(output_dir: str):
    output_path = pathlib.Path(output_dir)
    output_commands_file = output_path / 'compiled_commands.md'
    output_configs_file = output_path / 'compiled_configs.md'
    doc_data = get_documentation_maps(output_commands_file.name, output_configs_file.name)

    command_name_to_title_map = doc_data['command_name_to_title_map']
    config_name_to_title_map = doc_data['config_name_to_title_map']
    command_title_to_documentation_map = doc_data['command_title_to_documentation_map']
    config_title_to_documentation_map = doc_data['config_title_to_documentation_map']

    commands_documentation = ''
    configs_documentation = ''

    for title, doc in command_title_to_documentation_map.items():
        commands_documentation += doc
        commands_documentation += '\n\n'

    for title, doc in config_title_to_documentation_map.items():
        configs_documentation += doc
        configs_documentation += '\n\n'
    
    with open(output_commands_file, 'w', encoding='utf8') as outfile:
        outfile.write(commands_documentation)
    with open(output_configs_file, 'w', encoding='utf8') as outfile:
        outfile.write(configs_documentation)

def generate_json_documentation(output_dir: str):
    output_path = pathlib.Path(output_dir)
    output_file = output_path / 'sioyek_documentation.json'
    doc_data = get_documentation_maps('commands.md', 'configs.md')

    with open(output_file, 'w', encoding='utf8') as outfile:
        json.dump(doc_data, outfile, indent=4)


if __name__ == '__main__':
    parser = create_arg_parser()
    args = parser.parse_args(sys.argv[1:])

    generate_restructured_text_documentation(args.output_dir)
    generate_json_documentation(args.output_dir)

    # doc_data = get_documentation_maps('compiled_commands.md', 'compiled_configs.md')

    # command_name_to_title_map = doc_data['command_name_to_title_map']
    # config_name_to_title_map = doc_data['config_name_to_title_map']
    # command_title_to_documentation_map = doc_data['command_title_to_documentation_map']
    # config_title_to_documentation_map = doc_data['config_title_to_documentation_map']
#%%
# found = config_regex.findall(s)
# print(found)
# %%