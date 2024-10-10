# YeetMouse NixOS Flake

YeetMouse may be built using this folder's packaged Nix flake.
As the `flake.nix` file is in the `nix/` folder the flake is accessible under the `github:AndyFilter/YeetMouse?dir=nix` URL.

## NixOS Module Installation

You may install this flake by adding it to your NixOS configuration.
The following example shows the `inputs.yeetmouse` input and the
`yeetmouse.nixosModules.default` NixOS module being added to a system
in your `flake.nix` file.
    
```nix
{
  # Add this to your flake inputs
  # Note that `inputs.nixpkgs` assumes that you have an input called
  # `nixpkgs` and you might need to change it based on your `nixpkgs`
  # input's name.
  inputs.yeetmouse = {
    url = "github:AndyFilter/YeetMouse?dir=nix";
    inputs.nixpkgs.follows = "nixpkgs";
  };
  # <rest of your config> ...

  outputs = { nixpkgs, yeetmouse, ... }: {
    # This is an example of a NixOS system configuration
    nixosConfigurations.HOSTNAME = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        # Add the `yeetmouse` input's NixOS Module to your system's modules:
        yeetmouse.nixosModules.default
      ];
    };
 };
}
```

This will expose a new `hardware.yeetmouse` configuration option in your NixOS system's `config`,
which you can use to activate `yeetmouse`'s device driver, udev rules, and executable.

```nix
{
  hardware.yeetmouse = {
    enable = true;
    parameters = {
      # Alter the `parameters` to change the default settings of the `yeetmouse` driver
      # Check `nix/module.nix` to see all options
      Acceleration = 1.0;
    };
  };
}
```

Then, rebuild and switch into your new system (using `nixos-rebuild`). After a reboot, the
`yeetmouse` driver should be loaded for your connected mouse with the parameters you specified.

After restarting your system, to verify that the driver works start up the yeetmouse GUI:

```sh
sudo -E yeetmouse
```

## Manual Overlay Installation

Instead of adding the entire NixOS module, you may also manually add `pkgs.yeetmouse` as an
overlay to your system.

The following example also adds the `inputs.yeetmouse` input, but instad of addding the NixOS module
it adds the `yeetmouse.overlays.default` overlay.

```nix
{
  # Add this to your flake inputs
  # Note that `inputs.nixpkgs` assumes that you have an input called
  # `nixpkgs` and you might need to change it based on your `nixpkgs`
  # input's name.
  inputs.yeetmouse = {
    url = "github:AndyFilter/YeetMouse?dir=nix";
    inputs.nixpkgs.follows = "nixpkgs";
  };
  # <rest of your config> ...

  outputs = { nixpkgs, yeetmouse, ... }: {
    # This is an example of a NixOS system configuration
    nixosConfigurations.HOSTNAME = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      overlays = [
        # Add the `yeetmouse` input's overlay to your system's overlays:
        yeetmouse.overlays.default
      ];
    };
 };
}
```

This will only add a `pkgs.yeetmouse` package to your NixOS system configuration, and all further setup
needs to be done manually. Check the `module.nix` if you're unsure of how to use the `yeetmouse` package
manually.

```nix
{ pkgs, config, ... }:

let
  yeetmouse = pkgs.yeetmouse.override { inherit (config.boot.kernelPackages) kernel; };
in {
  # This installs the yeetmouse kernel module:
  boot.extraModulePackages = [ yeetmouse ];
  # This installs the yeetmouse GUI CLI package:
  environment.systemPackages = [ yeetmouse ];
  services.udev = {
    # This installs the yeetmouse udev rules:
    packages = [ yeetmouse ];

    # You may define a custom udev rule to apply default settings to the yeetmouse driver:
    extraRules = let
      echo = "${pkgs.coreutils}/bin/echo";
      yeetmouseConfig = pkgs.writeShellScriptBin "yeetmouseConfig" ''
        ${echo} "1.0" > /sys/module/leetmouse/parameters/Acceleration
        ${echo} "1" > /sys/module/leetmouse/parameters/update
      '';
    in ''
      SUBSYSTEMS=="usb|input|hid", ATTRS{bInterfaceClass}=="03", ATTRS{bInterfaceSubClass}=="01", ATTRS{bInterfaceProtocol}=="02", ATTRS{bInterfaceNumber}=="00", RUN+="${yeetmouseConfig}/bin/yeetmouseConfig"
    '';
  };
}
```

Since the `pkgs.yeetmouse` package contains a kernel module, it'll be built against a specific version
of the Linux kernel. By default this will be `pkgs.linxPackages.kernel` - the default version in nixpkgs.
To override this, `.override { inherit (config.boot.kernelPackages) kernel; }` is called in the example
above to use the selected kernel for your NixOS system instead.

## Flake Builds

This flake exposes a `packages.${system}.yeetmouse` output, which allows you to build
and test the `yeetmouse` package locally:

```sh
nix build .#yeetmouse --json --keep-failed
```
