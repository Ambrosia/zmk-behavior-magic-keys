{
  description = "ZMK behaviour for magic keys";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

    zephyr.url = "github:zmkfirmware/zephyr/v3.5.0+zmk-fixes";
    zephyr.flake = false;

    zmk.url = "github:zmkfirmware/zmk/v0.3.0";
    zmk.flake = false;

    zephyr-nix.url = "github:nix-community/zephyr-nix";
    zephyr-nix.inputs.zephyr.follows = "zephyr";
    zephyr-nix.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    {
      nixpkgs,
      zephyr-nix,
      zephyr,
      zmk,
      ...
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          zephyr-package = zephyr-nix.packages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = [
              zephyr-package.pythonEnv
              (zephyr-package.sdk.override { targets = [ "arm-zephyr-eabi" ]; })
              pkgs.cmake
              pkgs.ninja
              pkgs.gcc
              pkgs.dtc
            ];

            env.ZEPHYR_INCLUDE = "${zephyr}/include";
            env.ZMK_INCLUDE = "${zmk}/app/include";
            shellHook = ''
              cat > .clangd <<EOF
              CompileFlags:
                Add:
                  - -I${zephyr}/include
                  - -I${zmk}/app/include
                  - --target=arm-zephyr-eabi
                  - -DKERNEL
                  - -D__ZEPHYR__=1
              EOF
            '';
          };
        }
      );
    };
}
