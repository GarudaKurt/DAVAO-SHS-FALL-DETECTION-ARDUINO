def _arduino_sketch_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name + ".hex")
    sketch_files = [f.path for f in ctx.files.srcs if f.extension == "ino"]
    if not sketch_files:
        fail("The sketch must contain at least one .ino file")
    sketch = sketch_files[0]
    fqbn = ctx.var.get("fqbn", "")
    if not fqbn:
        fail("Missing board FQBN. Pass --define fqbn=vendor:arch:board")

    command = "arduino-cli compile --fqbn %s --output-dir \"$(@D)\" --export-binaries \"%s\" && copy /Y \"$(@D)\\*.hex\" \"%s\"" % (fqbn, sketch, out.path)
    ctx.actions.run_shell(
        inputs = ctx.files.srcs,
        outputs = [out],
        command = command,
        mnemonic = "ArduinoCompile",
        use_default_shell_env = True,
    )
    return [DefaultInfo(files = depset([out]))]

arduino_sketch = rule(
    implementation = _arduino_sketch_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
    },
)
