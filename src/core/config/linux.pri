include(common.pri)
include($$QTWEBENGINE_OUT_ROOT/qtwebengine-config.pri)
QT_FOR_CONFIG += gui-private webengine-private

gn_args += \
    use_cups=false \
    use_gconf=false \
    use_gio=false \
    use_gnome_keyring=false \
    use_kerberos=false \
    linux_use_bundled_binutils=false \
    use_nss_certs=true \
    use_openssl_certs=false

gcc:!clang: greaterThan(QT_GCC_MAJOR_VERSION, 5): gn_args += no_delete_null_pointer_checks=true

clang {
    clang_full_path = $$which($${QMAKE_CXX})
    # Remove the "/bin/clang++" part.
    clang_prefix = $$section(clang_full_path, /, 0, -3)
    gn_args += \
        is_clang=true \
        clang_use_chrome_plugins=false \
        clang_base_path=\"$${clang_prefix}\"

    linux-clang-libc++: gn_args += use_libcxx=true
} else {
    gn_args += \
        is_clang=false
}

cross_compile:!host_build {
    TOOLCHAIN_SYSROOT = $$[QT_SYSROOT]
    !isEmpty(TOOLCHAIN_SYSROOT): gn_args += target_sysroot=\"$${TOOLCHAIN_SYSROOT}\"
}

contains(QT_ARCH, "arm") {
    # Extract ARM specific compiler options that we have to pass to gn,
    # but let gn figure out a default if an option is not present.
    MTUNE = $$extractCFlag("-mtune=.*")
    !isEmpty(MTUNE): gn_args += arm_tune=\"$$MTUNE\"

    MFLOAT = $$extractCFlag("-mfloat-abi=.*")
    !isEmpty(MFLOAT): gn_args += arm_float_abi=\"$$MFLOAT\"

    MARCH = $$extractCFlag("-march=.*")

    MARMV = $$replace(MARCH, "armv",)
    !isEmpty(MARMV) {
        MARMV = $$split(MARMV,)
        MARMV = $$member(MARMV, 0)
        lessThan(MARMV, 6): error("$$MARCH architecture is not supported")
        gn_args += arm_version=$$MARMV
    }

    !lessThan(MARMV, 8) {
        gn_args += arm_use_neon=true
    } else {
        MFPU = $$extractCFlag("-mfpu=.*")
        !isEmpty(MFPU):contains(MFPU, ".*neon.*") {
            gn_args += arm_use_neon=true
        } else {
            gn_args += arm_use_neon=false
            # If the toolchain does not explicitly specify to use NEON instructions
            # we use arm_neon_optional for ARMv7
            equals(MARMV, 7): gn_args += arm_optionally_use_neon=true
        }
    }

    if(isEmpty(MARMV)|lessThan(MARMV, 7)):contains(QMAKE_CFLAGS, "-marm"): gn_args += arm_use_thumb=false
    else: contains(QMAKE_CFLAGS, "-mthumb"): gn_args += arm_use_thumb=true
}

contains(QT_ARCH, "mips") {
    MARCH = $$extractCFlag("-march=.*")
    !isEmpty(MARCH) {
        equals(MARCH, "mips32r6"): gn_args += mips_arch_variant=\"r6\"
        else: equals(MARCH, "mips32r2"): gn_args += mips_arch_variant=\"r2\"
        else: equals(MARCH, "mips32"): gn_args += mips_arch_variant=\"r1\"
    } else {
        contains(QMAKE_CFLAGS, "mips32r6"): gn_args += mips_arch_variant=\"r6\"
        else: contains(QMAKE_CFLAGS, "mips32r2"): gn_args += mips_arch_variant=\"r2\"
        else: contains(QMAKE_CFLAGS, "mips32"): gn_args += mips_arch_variant=\"r1\"
    }

    contains(QMAKE_CFLAGS, "-mmsa"): gn_args += mips_use_msa=true

    contains(QMAKE_CFLAGS, "-mdsp2"): gn_args += mips_dsp_rev=2
    else: contains(QMAKE_CFLAGS, "-mdsp"): gn_args += mips_dsp_rev=1
}

host_build {
    gn_args += custom_toolchain=\"$$QTWEBENGINE_OUT_ROOT/src/toolchain:host\"
    GN_HOST_CPU = $$gnArch($$QT_ARCH)
    gn_args += host_cpu=\"$$GN_HOST_CPU\"
    # Don't bother trying to use system libraries in this case
    gn_args += use_glib=false
    gn_args += use_system_libffi=false
} else {
    gn_args += custom_toolchain=\"$$QTWEBENGINE_OUT_ROOT/src/toolchain:target\"
    gn_args += host_toolchain=\"$$QTWEBENGINE_OUT_ROOT/src/toolchain:host\"
    GN_TARGET_CPU = $$gnArch($$QT_ARCH)
    cross_compile {
        gn_args += v8_snapshot_toolchain=\"$$QTWEBENGINE_OUT_ROOT/src/toolchain:v8_snapshot\"
        # FIXME: we should set host_cpu in case host-toolchain doesn't match os arch,
        # but currently we don't it available at this point
        gn_args += target_cpu=\"$$GN_TARGET_CPU\"
    } else {
        gn_args += host_cpu=\"$$GN_TARGET_CPU\"
    }
    !contains(QT_CONFIG, no-pkg-config) {
        # Strip '>2 /dev/null' from $$pkgConfigExecutable()
        PKGCONFIG = $$first($$list($$pkgConfigExecutable()))
        gn_args += pkg_config=\"$$PKGCONFIG\"
        gn_args += host_pkg_config=\"pkg-config\"
    }

    qtConfig(webengine-system-zlib): qtConfig(system-minizip) {
        gn_args += use_system_zlib=true use_system_minizip=true
        qtConfig(printing-and-pdf): gn_args += pdfium_use_system_zlib=true
    }
    qtConfig(webengine-system-png): gn_args += use_system_libpng=true
    qtConfig(system-jpeg): gn_args += use_system_libjpeg=true
    qtConfig(system-freetype): gn_args += use_system_freetype=true
    qtConfig(webengine-system-harfbuzz): gn_args += use_system_harfbuzz=true
    qtConfig(system-glib): gn_args += use_glib=false
    qtConfig(pulseaudio) {
        gn_args += use_pulseaudio=true
    } else {
        gn_args += use_pulseaudio=false
    }
    qtConfig(alsa) {
        gn_args += use_alsa=true
    } else {
        gn_args += use_alsa=false
    }
    packagesExist(libffi): gn_args += use_system_libffi=true
    else: gn_args += use_system_libffi=false
    !packagesExist(libpci): gn_args += use_libpci=false
    !packagesExist(xscrnsaver): gn_args += use_xscrnsaver=false

    qtConfig(system-libevent): gn_args += use_system_libevent=true
    qtConfig(system-libwebp):  gn_args += use_system_libwebp=true
    qtConfig(system-libxml2):  gn_args += use_system_libxml=true use_system_libxslt=true
    qtConfig(system-opus):     gn_args += use_system_opus=true
    qtConfig(system-snappy):   gn_args += use_system_snappy=true
    qtConfig(system-libvpx):   gn_args += use_system_libvpx=true
    qtConfig(system-icu):      gn_args += use_system_icu=true icu_use_data_file=false
    qtConfig(system-ffmpeg):   gn_args += use_system_ffmpeg=true
    qtConfig(system-re2):      gn_args += use_system_re2=true
    qtConfig(system-lcms2):    gn_args += use_system_lcms2=true

    # FIXME:
    #qtConfig(system-protobuf): gn_args += use_system_protobuf=true
    #qtConfig(system-jsoncpp):  gn_args += use_system_jsoncpp=true
    #qtConfig(system-libsrtp: gn_args += use_system_libsrtp=true
}
