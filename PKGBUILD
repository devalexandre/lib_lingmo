# Maintainer: Lingmo OS Team <team@lingmo.org>
# Contributor: devalexandre <alexandre@dev2learn.com>
pkgname=liblingmo
pkgver=2.0.0
pkgrel=1
pkgdesc="System libraries (network, audio, screen, bluetooth) of the Lingmo desktop"
arch=("x86_64")
url="https://github.com/LingmoOS/lib_lingmo"
license=("GPL")
depends=("qt6-base" "qt6-declarative" "qt6-sensors" "networkmanager-qt" "modemmanager-qt" "bluez-qt" "libkscreen" "kio" "libcanberra" "libpulse" "sound-theme-freedesktop")
makedepends=("cmake" "ninja" "extra-cmake-modules" "qt6-tools" "git")
provides=("$pkgname")
conflicts=("$pkgname")
source=("git+$url.git")
sha512sums=("SKIP")

build() {
    cmake -S lib_lingmo -B build -G Ninja \
        -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_BUILD_TYPE=None -DQT_NO_PRIVATE_MODULE_WARNING=ON
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
