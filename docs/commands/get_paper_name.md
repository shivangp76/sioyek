doc_body:
Returns the title of the current document. This is useful for scripting and probably not directly useful for end-users.

Example:
```python
from sioyek import Sioyek
s = Sioyek('')
paper_name = s.get_paper_name()
s.set_status_string(f'Current document name is: {paper_name}')
```