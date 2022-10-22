const std = @import("std");
const builtin = @import("builtin");
const Builder = std.build.Builder;
const path = std.fs.path;
const sep_str = path.sep_str;
const Target = std.zig.CrossTarget;

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();
    const target = b.standardTargetOptions(.{
        .whitelist = &[_]Target{
            .{
                .cpu_arch = .x86_64,
                .os_tag = .linux,
                .abi = .musl,
            },
            .{
                .cpu_arch = .x86_64,
                .os_tag = .windows,
                .abi = .gnu,
            },
        },
    });

    const exe = b.addExecutable("hf_csv_test", "src/main.c");
    exe.setBuildMode(mode);
    exe.setTarget(target);
    exe.linkSystemLibrary("c");

    b.default_step.dependOn(&b.addInstallDirectory(.{
        .source_dir = "res",
        .install_dir = .bin,
        .install_subdir = "res",
    }).step);
    
    b.default_step.dependOn(&exe.step);
    exe.install();

    const run = b.step("run", "Run the demo");
    const run_cmd = exe.run();
    run.dependOn(&run_cmd.step);
}
