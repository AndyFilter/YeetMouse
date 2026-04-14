{
  description = "A fork of a fork of the Linux mouse driver with acceleration. Now with GUI and some other improvements!";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = inputs @ { self, nixpkgs }: let
    inherit (inputs.nixpkgs) lib;
    packageInputs = {
      shortRev = if (self ? shortRev) then self.shortRev else self.dirtyRev;
    };
    eachSystem = lib.genAttrs ["aarch64-linux" "x86_64-linux"];
    overlay = final: prev: {
      yeetmouse = final.callPackage import ./package.nix packageInputs;
    };
  in {
    inherit inputs;
    nixosModules.default = import ./module.nix overlay;
    overlays.default = overlay;
    packages = eachSystem (system: {
      yeetmouse = nixpkgs.legacyPackages.${system}.callPackage ./package.nix packageInputs;
    });
  };
}
