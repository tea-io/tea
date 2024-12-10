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
      dtl = pkgs.stdenv.mkDerivation {
        name = "dtl";
        src = pkgs.fetchFromGitHub {
          repo = "dtl";
          owner = "cubicdaiya";
          rev = "32567bb9ec704f09040fb1ed7431a3d967e3df03";
          sha256 = "1zmjblwya8686h4l5yrf3kbcnfp8laq72ha12gk1nwv149335sxk";
        };

        installPhase = ''
          mkdir -p $out/include
          cp -r $src/dtl $out/include
        '';
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
          dtl
        ];

        buildPhase = ''
          make
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp server/server $out/bin/tea-server
          cp filesystem/filesystem $out/bin/tea-fs
        '';
      };
    };
  };
}
