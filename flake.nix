{
  description = "tea";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: {
    packages."x86_64-linux" = let
      pkgs = import nixpkgs {
        system = "x86_64-linux";
      };
    in {
      default = pkgs.stdenv.mkDerivation {
        name = "tea";
        src = ./.;

        nativeBuildInputs = [
          pkgs.gcc
          pkgs.clang-tools
          pkgs.clang
          pkgs.catch2_3

          pkgs.protobuf
          pkgs.pkg-config
          pkgs.fuse3
        ];

        buildPhase = ''
          make
        '';

        installPhase = ''
        '';
      };
    };
  };
}
