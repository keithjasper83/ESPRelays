Import("env")

if env["PIOENV"] == "native":
    env.Append(
        LINKFLAGS=["--coverage"],
        LIBS=["gcov"],
    )