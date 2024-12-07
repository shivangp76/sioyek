doc_body:
Displays a text prompt to the user and returns the entered text. This is used for scripting and is not directly useful for end-users.

Example:
```
from sioyek import Sioyek
s = sioyek.Sioyek('')
result = s.show_text_prompt('Enter a string:')
s.set_status_string(f'You entered {result}')
```