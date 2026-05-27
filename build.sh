SRC_DIR="src"
BUILD_DIR="build"

FLAGS="-ffreestanding -O2 -Wall -Wextra -fno-stack-protector -mno-red-zone -fno-pic -fno-pie -fno-builtin -mno-mmx -mno-sse -mno-sse2"
LDFLAGS="-n -static --oformat=elf64-x86-64 --no-warn-rwx-segments -z max-page-size=0x1000 -T linker.ld"

do_build() {
    echo "building aeternaOS..."
    mkdir -p "$BUILD_DIR"

    echo "[1/4] compiling NASM..."
    nasm -f elf64 "$SRC_DIR/boot.asm" -o "$BUILD_DIR/boot.o"

    echo "[2/4] compiling all .c files in /src..."
    OBJ_FILES="" # boot.o adds when linking
    
    for c_file in "$SRC_DIR"/*.c; do
        if [ -f "$c_file" ]; then
            filename=$(basename -- "$c_file")
            filename="${filename%.*}"
            
            echo "  -> $c_file"
            x86_64-linux-gnu-gcc -c "$c_file" -o "$BUILD_DIR/$filename.o" $FLAGS
            
            OBJ_FILES="$OBJ_FILES $BUILD_DIR/$filename.o"
        fi
    done

    echo "[3/4] linking kernel..."
    x86_64-linux-gnu-ld $LDFLAGS -o "$BUILD_DIR/kernel.bin" "$BUILD_DIR/boot.o" $OBJ_FILES

    if grub-file --is-x86-multiboot2 "$BUILD_DIR/kernel.bin"; then
        echo "  -> checking multiboot2 headers"
    else
        echo "  -> err: grub-file did not found multiboot2 in kernel.bin!"
    fi

    echo "[4/4] making iso..."
    mkdir -p "$BUILD_DIR/iso/boot/grub"
    cp "$BUILD_DIR/kernel.bin" "$BUILD_DIR/iso/boot/"
    
    if [ -f "iso_root/boot/grub/grub.cfg" ]; then
        cp "iso_root/boot/grub/grub.cfg" "$BUILD_DIR/iso/boot/grub/"
    else
        echo "ВНИМАНИЕ: grub.cfg was not found inside iso_root/boot/grub/!"
    fi
    
    grub-mkrescue -o aeternaos.iso "$BUILD_DIR/iso" 2>/dev/null
    echo "youre good to go now"
}

do_boot() {
    echo "booting system..."
    if [ -f "aeternaos.iso" ]; then
        qemu-system-x86_64 -cdrom aeternaos.iso -boot d -d cpu_reset,int -D direct_qemu.log
    else
        echo "err: aeternaos.iso was not found. have you built it?"
    fi
}

do_clean() {
    echo "cleaning..."
    
    if [ -d "build" ]; then
        rm -rf build
        echo "deleted /build"
    fi

    if [ -f "aeternaos.iso" ]; then
        rm -f aeternaos.iso
        echo "aeternaos.iso was removed"
    fi

    if [ -f "iso_root/boot/kernel.bin" ]; then
        rm -f iso_root/boot/kernel.bin
        echo " -> iso_root/boot/kernel.bin was removed"
    fi
}

case "$1" in
    cl) do_clean ;;
    bld) do_build ;;
    b)  do_boot  ;;
    cbb)   do_clean ; do_build ; do_boot ;;
    *)     echo "no valid args. usage: cl (clean) | bld (build) | b (boot) | cbb (everything at one command)" ;;
esac