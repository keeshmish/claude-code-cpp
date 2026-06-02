A minimal coding agent written in C++, built from scratch as part of the
[CodeCrafters](https://codecrafters.io) "Build Your Own Claude Code" challenge.

It runs a simple agentic loop: it sends your prompt to an LLM, and when the model
asks to use a tool, it executes that tool, feeds the result back, and repeats
until the model has a final answer. Three tools are supported:
 
- **Read**: read the contents of a file
- **Write**: write content to a file
- **Bash**: execute a shell command

**Please note:** This is a learning project, not a production tool. The agent executes shell
commands the model requests without sandboxing, so please do run it somewhere you don't
mind it touching the filesystem.
