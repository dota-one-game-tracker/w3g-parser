{
  description = "yolo dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        stdenv.cc      # <-- the important part (cc-wrapper)
        clang-tools    # clangd
        meson
        ninja
        pkg-config
      ];

      # Make sure Meson picks wrapper compilers
      shellHook = ''
        export CC=cc
        export CXX=c++
      '';
    };
  };
}

