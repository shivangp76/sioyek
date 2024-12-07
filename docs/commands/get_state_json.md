doc_body:
Returns a json string containing the current state of the application. The state is a list of json objects for each sioyek window. Each object contains the following fields:

- `document_checksum`: MD5 checksum of the document
- `document_path`: Path to the document
- `loaded_documents`: List of the paths of loaded documents
- `page_number`: Current page number 
- `searching`: Whether the application is currently searching
- `selected_text`: The currently selected text
- `window_height`: Height of the window
- `window_width`: Width of the window
- `window_id`: The id of the window
- `x_offset` and `y_offset`: Current position in document
- `x_offset_in_page` and `y_offset_in_page`: Current position in page
- `zoom_level`: Current zoom level

Example:
```python
from sioyek import sioyek
import json

s = sioyek.Sioyek('')
state = json.loads(s.get_state_json())
print(state['page_number'])
```