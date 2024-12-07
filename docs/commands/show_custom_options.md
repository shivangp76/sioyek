doc_body:
Given a `|`-separated list of options, displays the options to the user and returns the result. This is used for scripting and is not directly useful for end-users.

Example:
```
from sioyek import Sioyek
s = sioyek.Sioyek('')
result = s.show_custom_options('one|two|three')
s.set_status_string(f'You chose {result}')
```