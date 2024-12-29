import os
import google.auth
import google.oauth2.credentials
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload
from generate_video import video_base_path
import pathlib
import json
from tqdm import tqdm
import argparse
import sys
import movis as mv


CLIENT_SECRETS_FILE = os.environ.get('YOUTUBE_UPLOAD_CLIENT_SECRET_PATH')
SCOPES = ["https://www.googleapis.com/auth/youtube.upload"]
API_SERVICE_NAME = "youtube"
API_VERSION = "v3"
CACHE_FILE_PATH = pathlib.Path(__file__).parent.parent.parent / "bak" / "video_cache.json"
VIDEO_TIMESTAMPS_FILE_PATH = pathlib.Path(__file__).parent.parent.parent / "bak" / "video_file_timstamps.json"
VIDEO_FILE_PATH = pathlib.Path(__file__).parent.parent.parent / "bak" / "documentation.mp4"
VIDEO_ID_FILE_PATH = pathlib.Path(__file__).parent.parent.parent / "bak" / "documentation_video_id.txt"
COMPILED_DOCS_PATH = pathlib.Path(__file__).parent.parent / "build" / "sioyek_documentation.json"
VIDEO_DIRECTORY = pathlib.Path(__file__).parent.parent / "videos"
COMMAND_VIDEOS = VIDEO_DIRECTORY / "commands"
CONFIG_VIDEOS = VIDEO_DIRECTORY / "configs"

def create_arg_parser():
    parser = argparse.ArgumentParser(
        prog='sioyek_video_uploader',
        )

    parser.add_argument('--upload', action='store_true'  , help='Upload the generated video to youtube')
    return parser

def get_authenticated_service():
    credentials = None
    if os.path.exists("token.json"):
        credentials = Credentials.from_authorized_user_file("token.json", SCOPES)
    
    if not credentials or not credentials.valid:
        if credentials and credentials.expired and credentials.refresh_token:
            credentials.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(CLIENT_SECRETS_FILE, SCOPES)
            credentials = flow.run_local_server(port=0)
        
        with open("token.json", "w") as token:
            token.write(credentials.to_json())
    
    return build(API_SERVICE_NAME, API_VERSION, credentials=credentials)

def upload_video(youtube, video_file, title, description, tags, category_id, privacy_status):
    body = {
        "snippet": {
            "title": title,
            "description": description,
            "tags": tags,
            "categoryId": category_id,
        },
        "status": {
            "privacyStatus": privacy_status,
        },
    }

    # Call the API's videos.insert method to upload the video.
    media = MediaFileUpload(video_file, chunksize=-1, resumable=True)
    request = youtube.videos().insert(
        part="snippet,status",
        body=body,
        media_body=media
    )
    
    response = None
    while response is None:
        status, response = request.next_chunk()
        if status:
            print(f"Uploaded {int(status.progress() * 100)}%")
    
    return response['id']

def get_uploaded_videos():
    if CACHE_FILE_PATH.exists():
        json_data = json.loads(CACHE_FILE_PATH.read_text())
        return json_data["commands"], json_data["configs"]
    else:
        return [], []

def save_uploaded_video_data(uploaded_commands, uploaded_configs):
    CACHE_FILE_PATH.write_text(json.dumps({
        "commands": list(uploaded_commands),
        "configs": list(uploaded_configs),
    }))
    
if __name__ == '__main__':

    parser = create_arg_parser()
    args = parser.parse_args(sys.argv[1:])

    doc_json = json.loads(COMPILED_DOCS_PATH.read_text())
    command_name_to_file_name_map = doc_json['command_name_to_file_name_map']
    config_name_to_file_name_map = doc_json['config_name_to_file_name_map']
    command_file_names = set()
    config_file_names = set()

    for command_name, file_name in command_name_to_file_name_map.items():
        command_file_names.add(file_name)

    for config_name, file_name in config_name_to_file_name_map.items():
        config_file_names.add(file_name)

    
    command_video_to_upload = []
    config_video_to_upload = []

    for command_file_name in command_file_names:
        if ((COMMAND_VIDEOS / command_file_name).with_suffix('.mp4')).exists():
            video_file_path = (COMMAND_VIDEOS / command_file_name).with_suffix('.mp4')
            command_video_to_upload.append((video_file_path, command_file_name))
    for config_file_name in config_file_names:
        if ((CONFIG_VIDEOS / config_file_name).with_suffix('.mp4')).exists():
            video_file_path = (CONFIG_VIDEOS / config_file_name).with_suffix('.mp4')
            config_video_to_upload.append((video_file_path, config_file_name))


    uploaded_commands, uploaded_configs = get_uploaded_videos()

    uploaded_command_names = [x[0] for x in uploaded_commands]
    uploaded_config_names = [x[0] for x in uploaded_configs]

    command_video_to_upload = sorted(command_video_to_upload, key=lambda x: x[1])
    config_video_to_upload = sorted(config_video_to_upload, key=lambda x: x[1])




    clips = []

    def add_text_to_video(txt):
        text_scene = mv.layer.Composition(size=(1920, 1080), duration=1.0)

        text_scene.add_layer(mv.layer.Rectangle(text_scene.size, color='#000000'))  # Set background
        text_scene.add_layer(
            mv.layer.Text(txt, font_size=50, font_family='Consolas', color='#ffffff'),
            name='text',
            offset=0.0,
            position=(text_scene.size[0] / 2, text_scene.size[1] / 2),
            anchor_point=(0.0, 0.0),
            opacity=1.0, scale=1.0, rotation=0.0,
            blending_mode='normal')
        clips.append(text_scene)


    timestamps = dict()
    timestamps['commands'] = dict()
    timestamps['configs'] = dict()

    current_time = 0
    for video_file_path, file_name in tqdm(command_video_to_upload):

        timestamps['commands'][file_name] = current_time
        add_text_to_video(file_name)
        clips.append(mv.layer.Video(str(video_file_path)))
        current_time += 1 + clips[-1].duration

    for video_file_path, file_name in tqdm(config_video_to_upload):
        timestamps['configs'][file_name] = current_time
        add_text_to_video(file_name)
        clips.append(mv.layer.Video(str(video_file_path)))
        current_time += 1 + clips[-1].duration

    # print(current_time)
    final_video = mv.concatenate(clips)
    final_video.write_video(str(VIDEO_FILE_PATH))

    VIDEO_TIMESTAMPS_FILE_PATH.write_text(json.dumps(timestamps))
    if args.upload:
        youtube = get_authenticated_service()
        video_id = upload_video(youtube, VIDEO_FILE_PATH, 'test', None, None, None, 'unlisted')
        VIDEO_ID_FILE_PATH.write_text(video_id)


    # for video_file_path, file_name in tqdm(command_video_to_upload):
    #     if file_name not in uploaded_command_names:
    #         video_id = upload_video(youtube, video_file_path, file_name, None, None, None, 'unlisted')
    #         uploaded_commands.append((file_name, video_id))
    #         save_uploaded_video_data(uploaded_commands, uploaded_configs)

    # for video_file_path, file_name in tqdm(config_video_to_upload):
    #     if file_name not in uploaded_config_names:
    #         video_id = upload_video(youtube, video_file_path, file_name, None, None, None, 'unlisted')
    #         uploaded_configs.append((file_name, video_id))
    #         save_uploaded_video_data(uploaded_commands, uploaded_configs)



