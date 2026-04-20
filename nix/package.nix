{ shortRev ? "dev" }:
pkgs @ {
  lib,
  bash,
  coreutils,
  writeShellScript,
  makeDesktopItem,
  kernel ? pkgs.linuxPackages.kernel,
  kernelModuleMakeFlags ? pkgs.linuxPackages.kernelModuleMakeFlags,
  ...
}:

let
  stdenv = pkgs.llvmPackages_21.stdenv;
  mkPackage = overrides @ {
    kernel,
    ...
  }: (stdenv.mkDerivation rec {
    pname = "yeetmouse";
    version = shortRev;
    src = lib.fileset.toSource {
      root = ./..;
      fileset = ./..;
    };

    setSourceRoot = "export sourceRoot=$(pwd)/source";
    nativeBuildInputs = with pkgs; kernel.moduleBuildDependencies ++ [
      pkg-config
      makeWrapper
      autoPatchelfHook
      copyDesktopItems
      llvmPackages_21.clang-unwrapped
      llvmPackages_21.lld
      llvmPackages_21.llvm
    ];
    buildInputs = [
      pkgs.stdenv.cc.cc
      pkgs.glfw3
    ];

      makeFlags = [
        "LLVM=1"
        "LLVM_IAS=1"

        "CC=${pkgs.llvmPackages_21.clang-unwrapped}/bin/clang"
        "HOSTCC=${pkgs.llvmPackages_21.clang-unwrapped}/bin/clang"

        "LD=ld.lld"
        "HOSTLD=ld.lld"

        "-C"
        "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
        "M=$(sourceRoot)/driver"
      ];

    LD_LIBRARY_PATH = "/run/opengl-driver/lib:${lib.makeLibraryPath buildInputs}";

    postBuild = ''
      make "-j$NIX_BUILD_CORES" -C $sourceRoot/gui "M=$sourceRoot/gui" \
        "LIBS=-lglfw -lGL" \
        "CXXFLAGS=-Wno-sign-compare -Wno-unused-function -Wno-return-type -isystem $sourceRoot/gui/External"
    '';

    postInstall = let
      PATH = [ pkgs.zenity ];
    in /*sh*/''
      install -Dm755 $sourceRoot/gui/YeetMouseGui $out/bin/yeetmouse
      wrapProgram $out/bin/yeetmouse \
        --prefix PATH : ${lib.makeBinPath PATH}
    '';

    buildFlags = [ "modules" ];
    installFlags = [ "INSTALL_MOD_PATH=${placeholder "out"}" ];
    installTargets = [ "modules_install" ];

    desktopItems = [
      (makeDesktopItem {
        name = pname;
        exec = let
          xhost = "${pkgs.xorg.xhost}/bin/xhost";
        in writeShellScript "yeetmouse.sh" /*bash*/ ''
          if [ "$XDG_SESSION_TYPE" = "wayland" ]; then
            ${xhost} +SI:localuser:root
            pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "${pname}"
            ${xhost} -SI:localuser:root
          else
            pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "${pname}"
          fi
        '';
        type = "Application";
        desktopName = "Yeetmouse GUI";
        comment = "Yeetmouse Configuration Tool";
        categories = [
          "Settings"
          "HardwareSettings"
        ];
      })
    ];

    meta.mainProgram = "yeetmouse";
  }).overrideAttrs (prev: overrides);

  makeOverridable =
    f: origArgs:
    let
      origRes = f origArgs;
    in
    origRes // { override = newArgs: f (origArgs // newArgs); };
in
  makeOverridable mkPackage { inherit kernel; }
