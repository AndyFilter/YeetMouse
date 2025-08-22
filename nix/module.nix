yeetmouseOverlay:
{ pkgs, config, lib, ... }:

with lib;
let
  cfg = config.hardware.yeetmouse;

  degToRad = x: x * 0.017453292;
  floatRange = lower: upper: types.addCheck types.float (x: x >= lower && x <= upper);
      apply = x: if x != null then x else 0.0;

  parameterBasePath = "/sys/module/yeetmouse/parameters";

  rotationType = types.submodule {
    options = {
      angle = mkOption {
        type = floatRange (-180.0) 180.0;
        default = 0.0;
        apply = degToRad;
        description = "Rotation adjustment to apply to mouse inputs (in degrees)";
      };

      snappingAngle = mkOption {
        type = floatRange 0.0 179.9;
        default = 0.0;
        apply = degToRad;
        description = "Rotation angle to snap to";
      };

      snappingThreshold = mkOption {
        type = floatRange 0.0 179.9;
        default = 0.0;
        apply = degToRad;
        description = "Threshold until applying snapping angle";
      };
    };
  };

  modesType = types.attrTag {
    linear = mkOption {
      description = ''
        Simplest acceleration mode. Accelerates at a constant rate by multiplying acceleration.
        See [RawAccel: Linear](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#linear)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.0005 1.0;
            default = 0.15;
            description = "Linear acceleration multiplier";
          };
        };
      };
      apply = params: [
        {
          value = "1";
          param = "AccelerationMode";
        }
        {
          value = toString params.acceleration;
          param = "Acceleration";
        }
      ];
    };

    power = mkOption {
      description = ''
        Acceleration mode based on an exponent and multiplier as found in Source Engine games.
        See [RawAccel: Power](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#power)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.0005 5.0;
            default = 0.15;
            description = "Power acceleration pre-multiplier";
          };
          exponent = mkOption {
            type = floatRange 0.0005 1.0;
            default = 0.2;
            description = "Power acceleration exponent";
          };
          outputOffset = mkOption {
            type = floatRange 0.0 5.0;
            default = 1.0;
            description = "Speed output offset";
          };
        };
      };
      apply = params: [
        {
          value = "2";
          param = "AccelerationMode";
        }
        {
          value = toString params.acceleration;
          param = "Acceleration";
        }
        {
          value = toString params.exponent;
          param = "Exponent";
        }
        {
          value = toString params.outputOffset;
          param = "Midpoint";
        }
      ];
    };

    classic = mkOption {
      description = ''
        Acceleration mode based on an exponent and multiplier as found in Quake 3.
        See [RawAccel: Classic](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#classic)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.0005 5.0;
            default = 0.15;
            apply = toString;
            description = "Classic acceleration pre-multiplier";
          };
          exponent = mkOption {
            type = floatRange 2.0 5.0;
            default = 2.0;
            apply = toString;
            description = "Classic acceleration exponent";
          };
          useSmoothing = mkOption {
            type = bool;
            default = false;
            apply = toString;
            description = "Enables the ability to use smooth capping in the Classic curve";
            apply = x: if x then "1" else "0";
          };
          midpoint = mkOption {
            type = floatRange 0.1 50.0;
            default = 6.0;
            apply = toString;
            description = "Only used when useSmoothing is enabled, it a applies a smooth cap to the set value";
          };
        };
      };
      apply = params: [
        {
          value = "3";
          param = "AccelerationMode";
        }
        {
          value = toString params.classic.acceleration;
          param = "Acceleration";
        }
        {
          value = toString params.classic.exponent;
          param = "Exponent";
        }
        {
          value = toString params.classic.useSmoothing;
          param = "useSmoothing";
        }
        {
          value = toString params.classic.useSmoothing;
          param = "Midpoint";
        }
      ];
    };

    motivity = mkOption {
      description = ''
        Acceleration mode based on a sigmoid function with a set mid-point.
        See [RawAccel: Motivity](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#motivity)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.01 10.0;
            default = 0.15;
            apply = toString;
            description = "Motivity acceleration dividend";
          };
          start = mkOption {
            type = floatRange 0.1 50.0;
            default = 10.0;
            apply = toString;
            description = "Motivity acceleration mid-point";
          };
        };
      };
      apply = params: [
        {
          value = "4";
          param = "AccelerationMode";
        }
        {
          value = toString params.acceleration;
          param = "Acceleration";
        }
        {
          value = toString params.start;
          param = "Midpoint";
        }
      ];
    };

    synchronous = mkOption {
      description = ''
        This acceleration type is designed to match how we naturally perceive changes in speed, using a logarithmic sensitivity curve centered around a "synchronous speed." If the synchronous speed is set correctly, the sensitivity change will align with our intuitive estimation of speed differences.
        See [RawAccel: Synchronous](https://github.com/RawAccelOfficial/rawaccel/blob/master/doc/Guide.md#synchronous)
      '';
      type = types.submodule {
        options = {
          gamma = mkOption {
            type = floatRange 0.01 20.0;
            default = 0.3;
            apply = toString;
            description = "Expresses how fast the change occurs";
          };
          smoothness = mkOption {
            type = floatRange 0.1 20.0;
            default = 1.0;
            apply = toString;
            description = "Affects how fast the changes tails in and out";
          };
          motivity = mkOption {
            type = floatRange 1 10.0;
            default = 2.0;
            apply = toString;
            description = "Expresses how much change will occur";
          };
          syncspeed = mkOption {
            type = floatRange 0.01 20.0;
            default = 2.0;
            apply = toString;
            description = "Works a bit like offset";
          };
          useSmoothing = mkOption {
            type = types.bool;
            default = true;
            description = "Enable gain";
            apply = x: if x then "1" else "0";
          };
        };
      };
      apply = params: [
        {
          value = "5";
          param = "AccelerationMode";
        }
        {
          value = toString params.gamma;
          param = "Exponent";
        }
        {
          value = toString params.smoothness;
          param = "Midpoint";
        }
        {
          value = toString params.motivity;
          param = "Motivity";
        }
        {
          value = toString params.syncspeed;
          param = "Acceleration";
        }
        {
          value = params.useSmoothing;
          param = "UseSmoothing";
        }
      ];
    };

    natural = mkOption {
      description = ''
        Acceleration mode Natural features a concave curve which starts at 1 and approaches some maximum sensitivity. The sensitivity version of this curve can be found in the game Diabotical.
        See [RawAccel: Natural](https://github.com/RawAccelOfficial/rawaccel/blob/d179e22/doc/Guide.md#natural)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.001 5.0;
            default = 0.15;
            description = "Natural decay rate";
          };
          midpoint = mkOption {
            type = floatRange 0 50.0;
            default = 0;
            description = "Natural acceleration mid-point";
          };
          exponent = mkOption {
            type = floatRange 0.001 8.0;
            default = 2;
            description = "Natural acceleration limit (smoothness of the applied output curve)";
          };
          useSmoothing = mkOption {
            type = types.bool;
            default = false;
            description = "Enable Natural curve smoothing (Makes the curve smoother)";
            apply = x: if x then "1" else "0";
          };
        };
      };
      apply = params: [
        {
          value = "6";
          param = "AccelerationMode";
        }
        {
          value = toString params.acceleration;
          param = "Acceleration";
        }
        {
          value = toString params.midpoint;
          param = "Midpoint";
        }
        {
          value = toString params.exponent;
          param = "Exponent";
        }
        {
          value = params.useSmoothing;
          param = "UseSmoothing";
        }
      ];
    };

    jump = mkOption {
      description = ''
        Acceleration mode applying gain above a mid-point.
        Optionally, the transition mid-point can be smoothened and a smoothness may be applied to the whole sigmoid function.
        See [RawAccel: Jump](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#jump)
      '';
      type = types.submodule {
        options = {
          acceleration = mkOption {
            type = floatRange 0.01 10.0;
            default = 0.15;
            description = "Jump acceleration dividend";
          };
          midpoint = mkOption {
            type = floatRange 0.1 50.0;
            default = 0.15;
            description = "Jump acceleration mid-point";
          };
          smoothness = mkOption {
            type = floatRange 0.01 1.0;
            default = 0.2;
            description = "Jump curve smoothness (smoothness of the applied output curve)";
          };
          useSmoothing = mkOption {
            type = types.bool;
            default = true;
            description = "Enable Jump smoothing (whether the transition mid-point is smoothed out into the gain curve";
            apply = x: if x then "1" else "0";
          };
        };
      };
      apply = params: [
        {
          value = "7";
          param = "AccelerationMode";
        }
        {
          value = toString params.acceleration;
          param = "Acceleration";
        }
        {
          value = toString params.midpoint;
          param = "Midpoint";
        }
        {
          value = toString params.smoothness;
          param = "Exponent";
        }
        {
          value = params.useSmoothing;
          param = "UseSmoothing";
        }
      ];
    };

    lut = let
      tuple = ts: mkOptionType {
        name = "tuple";
        merge = mergeOneOption;
        check = xs: all id (zipListsWith (t: x: t.check x) ts xs);
        description = "tuple of" + concatMapStrings (t: " (${t.description})") ts;
      };
      lutVec = tuple [
        ((floatRange 0.0 100.0) // { description = "Input speed (x)"; })
        ((floatRange 0.0 100.0) // { description = "Output speed ratio (y)"; })
      ];
    in mkOption {
      description = ''
        Acceleration mode following a custom curve.
        The curve is specified using individual `[x, y]` points.
        See [RawAccel: Lookup Table](https://github.com/RawAccelOfficial/rawaccel/blob/5b39bb6/doc/Guide.md#look-up-table)
        The acceleration mode for custom curves is represented as a LUT as well. Use the Yeetmouse GUI to convert bezier curves to a LUT.
      '';
      type = types.submodule {
        options = {
          data = mkOption {
            type = types.listOf lutVec;
            default = [];
            apply = ls: map (t: "${toString t[0]},${toString t[1]}") ls;
            description = "Lookup Table data (a list of `[x, y]` points)";
          };
        };
      };
      apply = params: [
        {
          value = "8";
          param = "AccelerationMode";
        }
        {
          value = concatStringsSep ";" params.lut.data;
          param = "LutDataBuf";
        }
        {
          value = length params.lut.data;
          param = "LutSize";
        }
      ];
    };
  };

  yeetmouse = pkgs.yeetmouse.override {
    kernel = config.boot.kernelPackages.kernel;
  };
in {
  options.hardware.yeetmouse = {
    enable = mkOption {
      type = types.bool;
      default = false;
      description = "Enable yeetmouse kernel module to add configurable mouse acceleration";
    };

    sensitivity = let
      sensitivityValue = floatRange 0.01 10.0;
      anisotropyValue = types.submodule {
        description = "Anisotropic sensitivity, separating X and Y movement";
        options = {
          x = mkOption {
            type = sensitivityValue;
            description = "Horizontal sensitivity";
          };
          y = mkOption {
            type = sensitivityValue;
            description = "Vertical sensitivity";
          };
        };
      };
    in mkOption {
      type = types.either sensitivityValue anisotropyValue;
      default = 1.0;
      description = "Mouse base sensitivity";
      apply = sens: [
        {
          value = if isAttrs sens then toString sens.x else toString sens;
          param = "Sensitivity";
        }
        {
          value = if isAttrs sens then toString sens.y else toString sens;
          param = "SensitivityY";
        }
      ];
    };

    inputCap = mkOption {
      type = types.nullOr (floatRange 0.0 200.0);
      default = null;
      description = "Limit the maximum pointer speed before applying acceleration";
      apply = x: {
        value = if x != null then toString x else "0";
        param = "InputCap";
      };
    };

    outputCap = mkOption {
      type = types.nullOr (floatRange 0.0 100.0);
      default = null;
      description = "Cap maximum sensitivity.";
      apply = x: {
        value = if x != null then toString x else "0";
        param = "OutputCap";
      };
    };

    offset = mkOption {
      type = types.nullOr (floatRange (-50.0) 50.0);
      default = 0.0;
      description = "Acceleration curve offset";
      apply = x: {
        value = toString x;
        param = "Offset";
      };
    };

    preScale = mkOption {
      type = floatRange 0.01 10.0;
      default = 1.0;
      description = "Parameter to adjust for DPI";
      apply = x: {
        value = toString x;
        param = "PreScale";
      };
    };

    rotation = mkOption {
      type = rotationType;
      default = { };
      description = "Adjust mouse rotation input and optionally apply a snapping angle";
      apply = x: [
        {
          value = toString x.angle;
          param = "RotationAngle";
        }
        {
          value = toString x.snappingAngle;
          param = "AngleSnap_Angle";
        }
        {
          value = toString x.snappingThreshold;
          param = "AngleSnap_Threshold";
        }
      ];
    };

    mode = mkOption {
      type = modesType;
      default = {
        linear = { };
      };
      description = "Acceleration mode to apply and their parameters";
      apply = params: []
        ++ (optionals (params ? linear) params.linear)
        ++ (optionals (params ? power) params.power)
        ++ (optionals (params ? classic) params.classic)
        ++ (optionals (params ? motivity) params.motivity)
        ++ (optionals (params ? synchronous) params.synchronous)
        ++ (optionals (params ? natural) params.natural)
        ++ (optionals (params ? jump) params.jump)
        ++ (optionals (params ? lut) params.lut);
    };
  };

  config = mkIf cfg.enable {
    nixpkgs.overlays = [ yeetmouseOverlay ];

    boot.extraModulePackages = [ yeetmouse ];
    environment.systemPackages = [ yeetmouse ];
    services.udev = {
      extraRules = let
        echo = "${pkgs.coreutils}/bin/echo";
        yeetmouseConfig = let
          globalParams = [ cfg.inputCap cfg.outputCap cfg.offset cfg.preScale ];
          params = globalParams ++ cfg.sensitivity ++ cfg.rotation ++ cfg.mode;
          paramToString = entry: ''
            ${echo} "${entry.value}" > "${parameterBasePath}/${entry.param}"
          '';
        in pkgs.writeShellScriptBin "yeetmouseConfig" ''
          ${concatMapStrings (s: (paramToString s) + "\n") params}
          ${echo} "1" > /sys/module/yeetmouse/parameters/update
        '';
      in ''
        SUBSYSTEMS=="usb|input|hid", ATTRS{bInterfaceClass}=="03", ATTRS{bInterfaceSubClass}=="01", ATTRS{bInterfaceProtocol}=="02", ATTRS{bInterfaceNumber}=="00", RUN+="${yeetmouseConfig}/bin/yeetmouseConfig"
      '';
    };
  };
}
