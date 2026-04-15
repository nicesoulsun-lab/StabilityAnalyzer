######################## 通用配置模块 设置编译选项 ######################

CONFIG += c++11
#include(./BaseStruct/BaseStruct.pri)

CUSTOMNAME = $$member(TARGET)
# 自定义输出目录层级
equals(CUSTOMDIRLEVEL, ){
    # 默认../../../ 向上三级目录
    message($$CUSTOMNAME customdirlevel is  default ../../../ )
    CUSTOMDIRLEVEL =  ../../../
} else {
    message($$CUSTOMNAME customdirlevel is $$CUSTOMDIRLEVEL )
}
# 输出路径(可设置win32/unix等条件)
{
    # 路径
    msvc {
            #DESTDIR = ../../../bin-msvc
            DESTDIR = $$member(CUSTOMDIRLEVEL,0)bin-msvc
    } else {
            #DESTDIR = ../../../bin-mingw
            DESTDIR = $$member(CUSTOMDIRLEVEL,0)bin-mingw
    }
    #  同时生成debug与release
    CONFIG += debug_and_release build_all
    CONFIG(release, debug|release) {
            target_path = ./build_/dist
        }
        else {
            target_path = ./build_/debug
            #同时生成debug和release版本
            TARGET = $$member(TARGET,0)d
        }

        MOC_DIR = $$target_path/moc
        RCC_DIR = $$target_path/rcc
        UI_DIR = $$target_path/ui
        OBJECTS_DIR = $$target_path/obj
        
        # 确保库文件输出到DESTDIR目录
        DESTDIR = $$DESTDIR
}

# 判断是qml quick项目
# : 运算符：逻辑 AND 运算符，将许多条件连接在一起，并要求所有条件都为真
# | 运算符：逻辑 OR 运算符，将多个条件连接在一起，并且只需要其中一个为真

contains(QT, qml) : contains(QT, quick){
    message(Qt $$member(TARGET) has qml config)
    # app应用与插件应用的不同配置
    equals(TEMPLATE, app) {
        # 规避运行时compiler ahead错误 ~~没搞明白原因~~ #
        CONFIG -= qtquickcompiler
        # 添加插件依赖路径 qml中可以高亮
        QML_IMPORT_PATH += $$DESTDIR/customPlugin
        message($$QML_IMPORT_PATH)
    }
    equals(TEMPLATE, lib){
        CONFIG += plugin
        CONFIG -= qtquickcompiler
         # lib模式 输出文件到customPlugin目录下
        DESTDIR = $$DESTDIR/customPlugin/$$CUSTOMNAME
        {
            # 复制文件到指定目录 方法一
            #cpqmldir.files = qmldir
            #cpqmldir.path = $$DESTDIR
            #COPIES += cpqmldir

            # 复制文件到指定目录 方法二
            copy_file.input = qmldir
            copy_file.output = $$DESTDIR/qmldir
            # CONFIG指定的verbatim变量，表示完全拷贝文件内容
            # 如果不指定该变量，可以解析input文件中的qmake语法生成需要的文件
            copy_file.CONFIG = verbatim
            QMAKE_SUBSTITUTES += copy_file
        }
    }
} else {
    message(Qt $$member(TARGET) has no qml config)
}


# msvc 编译器
msvc {
    # 编码
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    # pdb
    #QMAKE_CFLAGS_RELEASE += /Z7
    #QMAKE_LFLAGS_RELEASE +=/debug /opt:ref
    #QMAKE_LFLAGS_RELEASE  += /INCREMENTAL:NO /DEBUG
    # 调试
    QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
mingw {
    # pdb
    #加入调试信息
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
    #禁止优化
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
    #release在最后link时默认有"-s”参数，表示"Omit all symbol information from the output file"，因此要去掉该参数
    QMAKE_LFLAGS_RELEASE += -mthreads
}

macx {
    #CONFIG += skd_no_version_check
}

## 去除config中多余的debug和release 输出(build_all时不会多次qmake)
## [https://blog.csdn.net/ihmhm12345/article/details/106082151] #
#defineReplace(remove_extra_config_parameter) {
#    configs = $$1
#    debug_and_release_params = # 匹配预选队列
#    keys = debug Debug release Release debug_and_release
#    for (iter, configs) {
#        contains(keys, $$iter) {
#            debug_and_release_params += $$iter
#        }
#    }

#    for (iter, debug_and_release_params) {
#        configs -= $$iter # 移除预选队列的属性
#    }

#    configs += $$last(debug_and_release_params) # 添加(保留)预选队列的最后属性

#    return($$configs)
#}

## 使用
#CONFIG = $$remove_extra_config_parameter($$CONFIG)


# 输出编译套件信息
message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(Qt Qml is installed in $$[QT_INSTALL_QML])
#message(the Application $$member(TARGET) will create in folder: $$target_path)
#message(the Application $$member(TARGET) out_pwd create in folder: $$OUT_PWD)
#message(the Application $$member(TARGET) pro_file_pwd create in folder: $$_PRO_FILE_PWD_)
message(the Application $$member(TARGET) tempalte is: $$member(TEMPLATE))
message(the Application $$member(TARGET) destdir is: $$member(DESTDIR))

