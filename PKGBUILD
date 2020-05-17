# Maintainer: Jens Guenther <dbotw@gmx.net>
pkgname=sdltrs
pkgver=master
pkgrel=1
pkgdesc="TRS-80 Model I/III/4/4P emulator for SDL2"
arch=(i686 x86_64)
url="https://gitlab.com/jengun/sdltrs"
license=('custom')
depends=('readline' 'sdl2')
makedepends=('autoconf' 'automake')
source=(${url}/-/archive/${pkgver}/${pkgname}-${pkgver}.tar.gz)
md5sums=('SKIP')

build() {
  cd ${srcdir}/${pkgname}-${pkgver}
  ./autogen.sh
  ./configure --prefix=/usr --enable-readline
  make
}

package() {
  cd ${srcdir}/${pkgname}-${pkgver}
  make DESTDIR=${pkgdir} install

  # Install license
  install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
