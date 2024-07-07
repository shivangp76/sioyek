related_commands: semantic_search_extractive

demo_code:

for_commands:
requires_pro: true

doc_body:
Performs a semantic search on the current document. This method produces a relatively lower quality result but costs a lot less than @command(semantic_search_extractive).

Explanation of how this method works for more technical users: This method simply splits the document into paragraph-sized chunks and embeds each chunk using an embedding model. Then it finds the chunks with most similarity to the query.