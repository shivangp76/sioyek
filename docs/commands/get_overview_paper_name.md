doc_body:
Returns the title of the paper in current overview window. This is useful for scripting and probably not directly useful for end-users.

Example:
```python
from sioyek import Sioyek
s = Sioyek('')
paper_name = s.get_overview_paper_name()
s.set_status_string(f'Current overview paper name is: {paper_name}')
```