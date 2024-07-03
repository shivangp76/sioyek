related_commands: get_config_value

Returns the current value of given config without displaying anything to the user. This is mainly useful for scripting.

Example:

```python
from sioyek import sioyek
s = sioyek.Sioyek('')
print(s.get_config_no_dialog('highlight_color_a')) # 'color3 0.94 0.64 1\n'
```