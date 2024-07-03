from sioyek import sioyek
import os
import pathlib
import pyscreenrec
import time
import shutil

video_base_path = pathlib.Path(__file__).parent.parent / "videos"
files_base_path = pathlib.Path(__file__).parent.parent / "files"
docs_base_path = pathlib.Path(__file__).parent.parent
markdown_file_path = pathlib.Path(r'C:\sioyek\sioyek-new\sioyek\docs\commands\goto_definition.md')
recorder = pyscreenrec.ScreenRecorder()

def start_recording(file_path):
    recorder.start_recording(file_path, 10)

def end_recording():
    recorder.stop_recording()

def show_run_command(s, command):
    s.command()

    for ch in command:
        if ch == ' ':
            s.type_text("' '")
        else:
            s.type_text(ch)
        time.sleep(0.1)
    
    time.sleep(0.5)
    s.control_menu('select')

def type_words(s, command):
    for ch in command:
        if ch == ' ':
            s.type_text("' '")
        else:
            s.type_text(ch)
        time.sleep(0.1)
    
    time.sleep(0.5)
    s.control_menu('select')


def get_video_file_path_for_markdown_file(markdown_file_path):
    return video_base_path / (markdown_file_path.relative_to(docs_base_path).with_suffix('.mp4'))

def generate_video_for_markdown_file(markdown_file_path):
    last_file_path = os.getenv('SIOYEK_LAST_FILE_PATH')
    local_database_path = os.getenv('SIOYEK_LOCAL_DATABASE_PATH')
    shared_database_path = os.getenv('SIOYEK_SHARED_DATABASE_PATH')

    try:
        os.remove(last_file_path, )
    except:
        pass
    try:
        os.remove(local_database_path)
    except:
        pass
    try:
        os.remove(shared_database_path)
    except:
        pass

    LAUNCH_ARGS = get_launch_args_for_markdown_file(markdown_file_path)
    video_file_path = get_video_file_path_for_markdown_file(markdown_file_path)
    RECORDING_FILE_NAME = str(video_file_path)
    with open(markdown_file_path, 'r', encoding='utf-8') as infile:
        markdown_content = infile.read()
    
    code_begin_index = markdown_content.find('```python')
    code_end_index = markdown_content.find('```', code_begin_index + 1)
    if code_begin_index >= 0:
        code_text = markdown_content[code_begin_index + 9:code_end_index].strip()
        print('^' * 80)
        print('generating video for ', markdown_file_path)
        exec(code_text)
        exec('s.quit()')
        return True
    return False

def get_launch_args_for_markdown_file(markdown_file_path):
    files_dir = files_base_path / markdown_file_path.relative_to(docs_base_path).with_suffix('')
    res = dict()

    if os.path.exists(files_dir):
        drawings_file = files_dir / 'drawings.sioyek.drawings'
        annotations_file = files_dir / 'annotations.sioyek.annotations'
        if os.path.exists(drawings_file):
            res['--force-drawing-path'] = drawings_file
        if os.path.exists(annotations_file):
            # copy annotations file with a .tmp suffix
            copied_annotations_file = annotations_file.with_suffix('.tmp')
            shutil.copy(annotations_file, copied_annotations_file)
            res['--force-annotations-path'] = copied_annotations_file
    return res
        # for file in os.listdir(files_dir):
        #     print(file)

def generate_video_for_new_markdown_files():
    print('-----------------')
    for file in os.listdir(docs_base_path / 'commands'):
        markdown_file_path = docs_base_path / 'commands' / file

        video_file_path = get_video_file_path_for_markdown_file(markdown_file_path)
        markdown_file_last_edit_time = os.path.getmtime(markdown_file_path)
        video_file_last_edit_time = os.path.getmtime(video_file_path) if os.path.exists(video_file_path) else None
        if (video_file_last_edit_time is None) or (markdown_file_last_edit_time > video_file_last_edit_time):
            generated = generate_video_for_markdown_file(markdown_file_path)
            if generated:
                print('generated video for ', markdown_file_path)

    # for base, dirs, files in os.walk(docs_base_path / 'commands'):
    #     file_path = pathlib.Path(base) / file

if __name__ == '__main__':
    generate_video_for_new_markdown_files()
    # generate_video_for_markdown_file(markdown_file_path)
    # print('-----')
    # print(video_base_path)
    # print(docs_base_path)
    # print(video_file_path)