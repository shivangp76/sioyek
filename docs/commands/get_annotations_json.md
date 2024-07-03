Returns the current document's annotations as a json string.

Example:
```python
from sioyek import Sioyek
import json
s = Sioyek('')
annotations = json.loads(s.get_annotations_json())
print(annotations)
```