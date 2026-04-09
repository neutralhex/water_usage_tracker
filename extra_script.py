Import("env")

env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)

try:
    env.AddCompilationDatabase("compile_commands.json")
except:
    pass
