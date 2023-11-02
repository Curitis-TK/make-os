
# 默认规则
default :
	../z_tools/make.exe img

# 文件规则
## 目标文件 : 所需文件

## ipl.nas -> ipl.bin
ipl.bin : ipl.nas Makefile
	../z_tools/nask.exe ipl.nas ipl.bin ipl.lst

## ipl.bin -> helloos.img
helloos.img : ipl.bin Makefile
	../z_tools/edimg.exe   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0   imgout:helloos.img


# 自定义目标和命令
## 编译 ipl.bin
asm :
	../z_tools/make.exe -r ipl.bin

## 构建 helloos.img
img :
	../z_tools/make.exe -r helloos.img

## 构建 helloos.img，复制 helloos.img 到 QEMU 的镜像目录并启动 QEMU
run :
	../z_tools/make.exe img
	copy helloos.img ..\z_tools\qemu\fdimage0.bin
	../z_tools/make.exe -C ../z_tools/qemu

## 构建 helloos.img 并将其写入虚拟软盘映像
install :
	../z_tools/make.exe img
	../z_tools/imgtol.com w a: helloos.img

## 删除生成的中间文件 ipl.bin 和 ipl.lst
clean :
	-del ipl.bin
	-del ipl.lst

# 执行 clean 目标并删除 helloos.img 文件
src_only :
	../z_tools/make.exe clean
	-del helloos.img
