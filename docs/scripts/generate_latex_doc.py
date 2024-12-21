import os
import pathlib
from dataclasses import dataclass
from typing import List
import re
import pypandoc
import tqdm
import json
import hashlib

DOCUMENTATION_PATH = pathlib.Path(__file__).parent.parent.absolute()
COMMANDS_PATH = DOCUMENTATION_PATH / "commands"
CONFIGS_PATH = DOCUMENTATION_PATH / "configs"
OUTPUT_DIR = DOCUMENTATION_PATH.parent / "bak" / "doc"
DOCUMENTATION_JSON_PATH = pathlib.Path(__file__).parent.parent.parent / "data" / "sioyek_documentation.json"
PANDOC_CACHE_PATH = DOCUMENTATION_PATH.parent / "bak" / "doc" / "pandoc_cache"
DOCUMENTATION_TEX_PATH = OUTPUT_DIR / 'sioyek_documentation.tex'

@dataclass
class Documentation:
    title: str
    content: str
    for_items: List[str]
    related_commands: List[str]
    related_configs: List[str]
    requires_pro: bool = False
    data_type: str = ''
    example: str = ''

    def get_title(self):
        if self.for_items != []:
            return f"{', '.join(self.for_items)}"
        else:
            return self.title

def get_attribute_list(attribute_name, file_content):
    attribute_index = file_content.find(attribute_name)
    if attribute_index >= 0:
        attribute_line = file_content[attribute_index + len(attribute_name):file_content.find('\n', attribute_index)].strip()
        if len(attribute_line) > 0:
            return attribute_line.split(' ')
    return []

def get_attribute(attribute_name, file_content):
    attribute_index = file_content.find(attribute_name)
    if attribute_index >= 0:
        attribute_line = file_content[attribute_index + len(attribute_name):file_content.find('\n', attribute_index)].strip()
        if len(attribute_line) > 0:
            return attribute_line.strip()
    return ''

def parse_command_doc(command_doc, title) -> Documentation:
    related_commands = get_attribute_list("related_commands:", command_doc)
    related_configs = get_attribute_list("related_configs:", command_doc)
    for_items = get_attribute_list("for_commands:", command_doc)
    requires_pro = get_attribute_list("requires_pro:", command_doc) != []
    content = command_doc[command_doc.find("doc_body:") + len("doc_body:"):].strip()
    return Documentation(title, content, for_items, related_commands, related_configs, requires_pro)

def parse_config_doc(config_doc, title) -> Documentation:
    related_commands = get_attribute_list("related_commands:", config_doc)
    related_configs = get_attribute_list("related_configs:", config_doc)
    type_string = get_attribute("type:", config_doc)
    requires_pro = get_attribute("requires_pro:", config_doc)
    example = get_attribute("example:", config_doc)

    if len(requires_pro) > 0:
        requires_pro = True

    for_items = get_attribute_list("for_configs:", config_doc)
    content = config_doc[config_doc.find("doc_body:") + len("doc_body:"):].strip()
    return Documentation(title, content, for_items, related_commands, related_configs, requires_pro, type_string, example)

def escape_title(title):
    return '\\texttt{' + title.replace("_", "\\_") + '}'

def escape_content(content, command_name_to_label_map, config_name_to_label_map):

    command_regex = re.compile(r'@command\((.*?)\)')
    config_regex = re.compile(r'@config\((.*?)\)')


    for match in command_regex.finditer(content):
        command_name = match.group(1)
        content = content.replace(f"@command({command_name})", f"`@command({command_name})`")

    for match in config_regex.finditer(content):
        config_name = match.group(1)
        content = content.replace(f"@config({config_name})", f"`@config({config_name})`")

    checksum = hashlib.md5(content.encode()).hexdigest()
    if os.path.exists(PANDOC_CACHE_PATH / f"{checksum}"):
        with open(PANDOC_CACHE_PATH / f"{checksum}", 'r', encoding='utf8') as infile:
            content = infile.read()
            content = content.replace('\r', '')
    else:
        content = pypandoc.convert_text(content, to='latex', format='md')
        with open(PANDOC_CACHE_PATH / f"{checksum}", 'w', encoding='utf8') as outfile:
            outfile.write(content.replace('\r', ''))

    # replace @command and @config with hyperlinks
    for match in command_regex.finditer(content):
        command_name = match.group(1)
        command_name_ = command_name.replace("\\_", '_')
        if command_name_ in command_name_to_label_map:
            content = content.replace(f"@command({command_name})", f"\\hyperref[{command_name_to_label_map[command_name_]}]{{{command_name}}}")
        else:
            content = content.replace(f"@command({command_name})", f"\\textcolor{{red}}{{\\texttt{{{command_name}}}???}}")
    
    for match in config_regex.finditer(content):
        config_name = match.group(1)
        config_name_ = config_name.replace('\\_', '_')
        if config_name_ in config_name_to_label_map:
            content = content.replace(f"@config({config_name})", f"\\hyperref[{config_name_to_label_map[config_name_]}]{{{config_name}}}")
        else:
            content = content.replace(f"@config({config_name})", f"\\textcolor{{red}}{{\\texttt{{{config_name}}}???}}")


    return content

def get_latex_documentation():
    command_docs: List[Documentation] = []
    config_docs: List[Documentation] = []

    for file in os.listdir(COMMANDS_PATH):
        with open(COMMANDS_PATH / file, "r") as f:
            content = f.read()
            title = file.split(".")[0]
            command_docs.append(parse_command_doc(content, title))

    for file in os.listdir(CONFIGS_PATH):
        with open(CONFIGS_PATH / file, "r") as f:
            content = f.read()
            title = file.split(".")[0]
            config_docs.append(parse_config_doc(content, title))
        
    command_name_to_label_map = dict()
    config_name_to_label_map = dict()

    for command in command_docs:
        if command.for_items != []:
            for item in command.for_items:
                command_name_to_label_map[item] = "command:" + command.title
        else:
            command_name_to_label_map[command.title] = "command:" + command.title
    
    for config in config_docs:
        if config.for_items != []:
            for item in config.for_items:
                config_name_to_label_map[item] = "config:" + config.title
        else:
            config_name_to_label_map[config.title] = "config:" + config.title
    
    with open(DOCUMENTATION_JSON_PATH, "r", encoding='utf8') as infile:
        documentation_json = json.load(infile)

    command_name_to_file_map = documentation_json["command_name_to_file_name_map"]
    config_name_to_file_map = documentation_json["config_name_to_file_name_map"]

    documentation = r'''
\documentclass{article}
\usepackage{hyperref}
\usepackage{tcolorbox}

\usepackage{tocloft} % Load the package

\setlength{\cftsubsecnumwidth}{2.75em} % Increase the width to add more space

\usepackage{longtable}
\usepackage{array}
\usepackage{booktabs}
\usepackage{calc}

\PassOptionsToPackage{unicode=true}{hyperref} % options for packages loaded elsewhere
\PassOptionsToPackage{hyphens}{url}

\pdfstringdefDisableCommands{%
  \def\\{}%
  \def\newline{}%
  \def\texttt#1{<#1>}%
}

\newcounter{MarkdownId}

\newenvironment{markdown}%
    {\stepcounter{MarkdownId}%
        \VerbatimEnvironment\begin{VerbatimOut}{tmp/tmp\theMarkdownId.md}%
    }%
    {\end{VerbatimOut}%
        \immediate\write18{pandoc tmp/tmp\theMarkdownId.md -t latex -o tmp/tmp\theMarkdownId.tex}%
        \input{tmp/tmp\theMarkdownId.tex}%
    }

\usepackage{lmodern}
\usepackage{amssymb,amsmath}
\usepackage{ifxetex,ifluatex}
\ifnum 0\ifxetex 1\fi\ifluatex 1\fi=0 % if pdftex
  \usepackage[T1]{fontenc}
  \usepackage[utf8]{inputenc}
  \usepackage{textcomp} % provides euro and other symbols
\else % if luatex or xelatex
  \usepackage{unicode-math}
  \defaultfontfeatures{Ligatures=TeX,Scale=MatchLowercase}
\fi
% use upquote if available, for straight quotes in verbatim environments
\IfFileExists{upquote.sty}{\usepackage{upquote}}{}
% use microtype if available
\IfFileExists{microtype.sty}{%
\usepackage[]{microtype}
\UseMicrotypeSet[protrusion]{basicmath} % disable protrusion for tt fonts
}{}
\IfFileExists{parskip.sty}{%
\usepackage{parskip}
}{% else
\setlength{\parindent}{0pt}
\setlength{\parskip}{6pt plus 2pt minus 1pt}
}

\urlstyle{same}  % don't use monospace font for urls
\usepackage{color}
\usepackage{fancyvrb}
\newcommand{\VerbBar}{|}
\newcommand{\VERB}{\Verb[commandchars=\\\{\}]}
\DefineVerbatimEnvironment{Highlighting}{Verbatim}{commandchars=\\\{\}}
% Add ',fontsize=\small' for more characters per line
\newenvironment{Shaded}{}{}
\newcommand{\KeywordTok}[1]{\textcolor[rgb]{0.00,0.44,0.13}{\textbf{#1}}}
\newcommand{\DataTypeTok}[1]{\textcolor[rgb]{0.56,0.13,0.00}{#1}}
\newcommand{\DecValTok}[1]{\textcolor[rgb]{0.25,0.63,0.44}{#1}}
\newcommand{\BaseNTok}[1]{\textcolor[rgb]{0.25,0.63,0.44}{#1}}
\newcommand{\FloatTok}[1]{\textcolor[rgb]{0.25,0.63,0.44}{#1}}
\newcommand{\ConstantTok}[1]{\textcolor[rgb]{0.53,0.00,0.00}{#1}}
\newcommand{\CharTok}[1]{\textcolor[rgb]{0.25,0.44,0.63}{#1}}
\newcommand{\SpecialCharTok}[1]{\textcolor[rgb]{0.25,0.44,0.63}{#1}}
\newcommand{\StringTok}[1]{\textcolor[rgb]{0.25,0.44,0.63}{#1}}
\newcommand{\VerbatimStringTok}[1]{\textcolor[rgb]{0.25,0.44,0.63}{#1}}
\newcommand{\SpecialStringTok}[1]{\textcolor[rgb]{0.73,0.40,0.53}{#1}}
\newcommand{\ImportTok}[1]{#1}
\newcommand{\CommentTok}[1]{\textcolor[rgb]{0.38,0.63,0.69}{\textit{#1}}}
\newcommand{\DocumentationTok}[1]{\textcolor[rgb]{0.73,0.13,0.13}{\textit{#1}}}
\newcommand{\AnnotationTok}[1]{\textcolor[rgb]{0.38,0.63,0.69}{\textbf{\textit{#1}}}}
\newcommand{\CommentVarTok}[1]{\textcolor[rgb]{0.38,0.63,0.69}{\textbf{\textit{#1}}}}
\newcommand{\OtherTok}[1]{\textcolor[rgb]{0.00,0.44,0.13}{#1}}
\newcommand{\FunctionTok}[1]{\textcolor[rgb]{0.02,0.16,0.49}{#1}}
\newcommand{\VariableTok}[1]{\textcolor[rgb]{0.10,0.09,0.49}{#1}}
\newcommand{\ControlFlowTok}[1]{\textcolor[rgb]{0.00,0.44,0.13}{\textbf{#1}}}
\newcommand{\OperatorTok}[1]{\textcolor[rgb]{0.40,0.40,0.40}{#1}}
\newcommand{\BuiltInTok}[1]{#1}
\newcommand{\ExtensionTok}[1]{#1}
\newcommand{\PreprocessorTok}[1]{\textcolor[rgb]{0.74,0.48,0.00}{#1}}
\newcommand{\AttributeTok}[1]{\textcolor[rgb]{0.49,0.56,0.16}{#1}}
\newcommand{\RegionMarkerTok}[1]{#1}
\newcommand{\InformationTok}[1]{\textcolor[rgb]{0.38,0.63,0.69}{\textbf{\textit{#1}}}}
\newcommand{\WarningTok}[1]{\textcolor[rgb]{0.38,0.63,0.69}{\textbf{\textit{#1}}}}
\newcommand{\AlertTok}[1]{\textcolor[rgb]{1.00,0.00,0.00}{\textbf{#1}}}
\newcommand{\ErrorTok}[1]{\textcolor[rgb]{1.00,0.00,0.00}{\textbf{#1}}}
\newcommand{\NormalTok}[1]{#1}
\setlength{\emergencystretch}{3em}  % prevent overfull lines
\providecommand{\tightlist}{%
  \setlength{\itemsep}{0pt}\setlength{\parskip}{0pt}}

\begin{document}
\sloppy
\title{Sioyek Documentation}
\date{}
\maketitle
\tableofcontents
\section{Commands}
'''

    def handle_documentation(command, type_string):
        nonlocal documentation
        style = 'arc=0mm, colback=white, colframe=black!75!black, coltitle=lightgray, fonttitle=\\bfseries'

        def get_item_link(type_string, map, item):
            if item not in map:
                return f"\\textcolor{{red}}{{\\hyperref[{type_string}:{map.get(item, '')}]{{{escape_title(item)}}}???}}" 
            return f"\\hyperref[{type_string}:{map.get(item, '')}]{{{escape_title(item)}}}" 

        if len(command.related_commands) > 0:
            related_commands = '\\begin{tcolorbox}[' + style + ', title=\\texttt{Related Commands}]\\begin{flushleft}\n\n' + ', '.join([get_item_link("command", command_name_to_file_map, c) for c in command.related_commands]) + '\n\n\\end{flushleft}\\end{tcolorbox}'
        else:
            related_commands = ""
        if len(command.related_configs) > 0:
            related_configs = '\\begin{tcolorbox}[' + style + ', title=\\texttt{Related Configs}]\\begin{flushleft}\n\n' + ', '.join([get_item_link("config", config_name_to_file_map, c) for c in command.related_configs]) + '\n\n\\end{flushleft}\\end{tcolorbox}'
        else:
            related_configs = ""

        example_text = ''
        if type_string == 'config' and command.example != '':
            example_text = f'\\begin{{tcolorbox}}[arc=0mm, title=Example]{escape_title(command.example)}\\end{{tcolorbox}}'


        config_type_string = ''
        if command.data_type != '':
            config_type_string = '\\textcolor{gray}{\\texttt{[' + escape_title(command.data_type) + ']}} '

        documentation += f'''
\\subsection{{{config_type_string + escape_title(command.get_title())}}}\\label{{{type_string}:{command.title}}}\hypertarget{{{type_string}:{command.title}}}{{}}

{escape_content(command.content, command_name_to_label_map, config_name_to_label_map)}
{example_text}
{related_commands}{related_configs}
'''

    index = 0
    for command in tqdm.tqdm(command_docs):
        index += 1
        handle_documentation(command, 'command')
        # if index == 10:
        #     break

    documentation += r'''
\section{Configs}
'''

    index = 0
    for config in tqdm.tqdm(config_docs):
        index += 1
        handle_documentation(config, 'config')
#         documentation += f'''
# \\subsection{{{escape_title(config.get_title())}}}\\label{{config:{config.title}}}
# {escape_content(config.content, command_name_to_label_map, config_name_to_label_map)}\\hypertarget{{config:{config.title}}}{{}}
# '''
        # if index == 10:
        #     break

    documentation += r'''
\end{document}
    '''


    return documentation 

def latex_main():
    docs = get_latex_documentation()
    docs = docs.replace('\r', '')
    with open(DOCUMENTATION_TEX_PATH, 'w', encoding='utf8') as outfile:
        outfile.write(docs)

if __name__ == '__main__':
    latex_main()

#%%