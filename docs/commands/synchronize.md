related_commands: 

demo_code:

for_commands:
requires_pro: true

doc_body:
Synchronize a desynchronized file with the server. On rare occasions files might become desynchronized where sioyek can not automatically decide whether to upload annotations to sioyek servers or not. For example suppose you have two copies of the same file on two different devices and with different annotations, and then you upload one of them to sioyek servers. When the other file is opened on the other device, sioyek will not automatically decide to upload the annotations to the server (because the user might not want that, e.g. perhaps those annotations are very sensitive data that you do not want uploaded). In such situations the other file is in desynchronized state and you can use this command to synchronize it with the server. This command will upload the annotations to the server if they are not already there.