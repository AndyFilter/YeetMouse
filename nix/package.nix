{ shortRev ? "dev" }:
pkgs @ {
  lib,
  bash,
  stdenv,
  coreutils,
  writeShellScript,
  makeDesktopItem,
  kernel ? pkgs.linuxPackages.kernel,
  kernelModuleMakeFlags ? pkgs.linuxPackages.kernelModuleMakeFlags,
  ...
}:

let
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
      makeWrapper
      autoPatchelfHook
      copyDesktopItems
    ];
    buildInputs = [
      stdenv.cc.cc.lib
      pkgs.glfw3
    ];

    makeFlags = kernelModuleMakeFlags ++ [
      "KBUILD_OUTPUT=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
      "-C"
      "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
      "M=$(sourceRoot)/driver"
    ];

    preBuild = ''
      cp $sourceRoot/driver/config.sample.h $sourceRoot/driver/config.h
    '';

    LD_LIBRARY_PATH = "/run/opengl-driver/lib:${lib.makeLibraryPath buildInputs}";

    postBuild = ''
      make "-j$NIX_BUILD_CORES" -C $sourceRoot/gui "M=$sourceRoot/gui" "LIBS=-lglfw -lGL"
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
