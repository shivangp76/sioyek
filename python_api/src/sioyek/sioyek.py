from contextlib import redirect_stdout
from dataclasses import dataclass
from functools import lru_cache
import os
import regex
import sqlite3
import subprocess
import sys
import math
from collections import defaultdict
from PyQt5.QtNetwork import QLocalSocket
from PyQt5.QtCore import QByteArray, QDataStream, QIODevice
import json
from .base import SioyekBase
import time

import fitz

def parse_typed_config_str(lines):
    results = []
    for line in lines.split('\n'):
        if len(line) > 0:
            index_of_first_space = line.find(' ')
            type_str = line[:index_of_first_space]
            config_str = line[index_of_first_space+1:].strip()
            
            if type_str == "bool":
                results.append(bool(int(config_str)))
            elif type_str == "int":
                results.append(int(config_str))
            elif type_str == "float":
                results.append(float(config_str))
            elif type_str in ["color3", "color4", "fvec2"]:
                parts = config_str.split(' ')
                results.append(tuple([float(part) for part in parts]))
            elif type_str in ["string", "filepath", "folderpath", "macro"]:
                results.append(config_str)
            elif type_str == "ivec2":
                parts = config_str.split(' ')
                results.append(tuple([int(part) for part in parts]))
            elif type_str == "enablerectangle":
                parts = config_str.split(' ')
                enabled = bool(int(parts[0]))
                coords = tuple([int(part) for part in parts[1:]])
                results.append((enabled, coords))
            else:
                results.append(type_str)
    if len(results) == 1:
        return results[0]
    else:
        return results



class Sioyek(SioyekBase):

    def __init__(self, sioyek_path='', local_database_path=None, shared_database_path=None, last_file_path=None, force_binary=False, launch_if_not_exists=False, launch_args=dict()):

        self.path = sioyek_path
        self.is_dummy_mode = False

        self.local_database = None
        self.shared_database = None
        self.cached_path_hash_map = None
        self.highlight_embed_method = 'fitz'
        # run the binary sioyek command instead of using local sockets
        # it is much slower and is kept only for possible backward incompatibilities
        self.force_binary = force_binary
        self.connected = False
        self.socket = None
        self.last_file_path = last_file_path

        if not force_binary:
            self.try_to_connect()
            # self.socket = QLocalSocket()
            # self.socket.connectToServer('sioyek')
            # if self.socket.waitForConnected(1000):
            #     self.connected = True


        self.local_database_path = local_database_path
        self.shared_database_path = shared_database_path

        if not self.connected and launch_if_not_exists:

            if len(self.path) == 0:
                self.path = os.getenv('SIOYEK_EXE_PATH')

            if local_database_path == None and os.getenv('SIOYEK_LOCAL_DATABASE_PATH') != None:
                local_database_path = os.getenv('SIOYEK_LOCAL_DATABASE_PATH')

            if shared_database_path == None and os.getenv('SIOYEK_SHARED_DATABASE_PATH') != None:
                shared_database_path = os.getenv('SIOYEK_SHARED_DATABASE_PATH')

            if last_file_path == None and os.getenv('SIOYEK_LAST_FILE_PATH') != None:
                last_file_path = os.getenv('SIOYEK_LAST_FILE_PATH')

            args = [self.path]

            if local_database_path != None:
                self.local_database_path = local_database_path
                args.extend(['--local-database-path', local_database_path])

            if shared_database_path != None:
                self.shared_database_path = shared_database_path
                args.extend(['--shared-database-path', shared_database_path])

            if last_file_path != None:
                args.extend(['--last-file-path', last_file_path])
            
            for key, value in launch_args.items():
                args.extend([key, value])
            
            subprocess.Popen(args)
            for i in range(10):
                time.sleep(1)
                if not self.connected:
                    self.try_to_connect()
                else:
                    break
            if not self.connected:
                raise Exception('Could not connect to sioyek')

        if self.local_database_path != None:
            self.local_database = sqlite3.connect(self.local_database_path)

        if self.shared_database_path != None:
            self.shared_database = sqlite3.connect(self.shared_database_path)
    
    def try_to_connect(self):
        self.socket = QLocalSocket()
        self.socket.connectToServer('sioyek')
        if self.socket.waitForConnected(1000):
            self.connected = True

    def should_use_local_socket(self):
        return (not self.force_binary) and (self.connected)

    def set_highlight_embed_method(self, method):
        self.highlight_embed_method = method

    def get_local_database(self):
        return self.local_database

    def get_shared_database(self):
        return self.shared_database

    def set_dummy_mode(self, mode):
        '''
        dummy mode prints commands instead of executing them on sioyek
        '''
        self.is_dummy_mode = mode

    def wait_for_render(self, window_id=None):
        self.wait_for_renders_to_finish(None, window_id=window_id)

    def wait_for_search(self, window_id=None):
        self.wait_for_search_to_finish(None, window_id=window_id)

    def wait_for_index(self, window_id=None):
        self.wait_for_indexing_to_finish(None, window_id=window_id)

    def annotations(self, annot_type):
        annotations = json.loads(self.get_annotations_json())
        if annot_type != "":
            return annotations[annot_type]
        else:
            return annotations

    def state(self, arg="", window_id=None, return_all=False):
        states = json.loads(self.get_state_json())
        if return_all:
            if arg == "":
                return states
            else:
                return [state[arg] for state in states]

        state = None
        if window_id == None:
            state = states[0]
        else:
            for s in states:
                if s['window_id'] == window_id:
                    state = s
                    break

        if arg == "":
            return state
        else:
            return state[arg]


    def confs(self, conf_name):
        return self.get_config_no_dialog(conf_name)

    def confv(self, conf_name):
        s = self.get_config_no_dialog(conf_name)
        index = s.find(' ')
        return s[index+1:]

    def conf(self, conf_name):
        return parse_typed_config_str(self.confs(conf_name))
    
    def get_path_hash_map(self):

        if self.cached_path_hash_map == None:
            query = 'SELECT * from document_hash'
            cursor = self.get_local_database().execute(query)
            results = cursor.fetchall()
            res = dict()
            for _, path, hash_ in results:
                res[path] = hash_
            self.cached_path_hash_map = res

        return self.cached_path_hash_map

    def run_command(self, command_name, text=None, focus=False, wait=True, window_id=None):

        def escape_arg(arg):
            if type(arg) == str:
                return arg.replace('\\', '\\\\')
            else:
                return str(arg)
        if type(text) == list or type(text) == tuple:
            if len(text) == 0:
                text = None
            elif len(text) == 1 and type(text[0]) == list or type(text[0]) == tuple or text[0] is None:
                text = text[0]

        if type(text) == list or type(text) == tuple:
            text = ["'" + escape_arg(t) + "'" for t in text]
            args_string = ', '.join(text)
            params = [self.path, '--execute-command', command_name + '(' + args_string + ')']
        else:
            if text == None:
                params = [self.path, '--execute-command', command_name]
            else:
                params = [self.path, '--execute-command', command_name, '--execute-command-data', str(text)]
        
        if focus == False:
            params.append('--nofocus')

        if wait:
            params.append('--wait-for-response')
        
        if self.local_database_path:
            params.extend(['--local-database-path', self.local_database_path])

        if self.shared_database_path:
            params.extend(['--shared-database-path', self.shared_database_path])

        if self.last_file_path:
            params.extend(['--last-file-path', self.last_file_path])

        if window_id != None:
            params.append('--window-id')
            params.append(str(window_id))
        
        if self.is_dummy_mode:
            print('dummy mode, executing: ', params)
            return ""
        else:
            if self.should_use_local_socket():
                data = QByteArray()
                data_stream = QDataStream(data, QIODevice.WriteOnly)
                data_stream.writeInt(len(params))
                for param in params:
                    data_stream.writeQString(param)
                self.socket.write(data)
                self.socket.flush()
                self.socket.waitForBytesWritten(1000)
                if wait:
                    self.socket.waitForReadyRead(-1)
                    response = self.socket.readAll()
                    return str(response, 'utf-8')
            else:
                subprocess.run(params)
                return ""

    
    # def get_document(self, path):
    #     return Document(path, self)
    
    def close(self):
        self.local_database.close()
        self.shared_database.close()


    def statusbar_output(self):

        class StatusBarOutput(object):
            def __init__(self, sioyek):
                self.old_stdout = sys.stdout
                self.sioyek = sioyek
            
            def write(self, text):
                if text.strip() != '':
                    self.sioyek.set_status_string(text)
        return redirect_stdout(StatusBarOutput(self))

@dataclass
class DocumentPos:
    page: int
    offset_x: float
    offset_y: float

@dataclass
class AbsoluteDocumentPos:
    offset_x: float
    offset_y: float
