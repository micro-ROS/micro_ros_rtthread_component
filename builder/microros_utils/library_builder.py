import rtconfig
import os, sys
import shutil
import stat
from distutils.dir_util import copy_tree

from .utils import run_cmd, rmtree
from .repositories import Repository, Sources

class Build:
    def __init__(self, library_folder, packages_folder, distro, env):

        self.temp_folder = os.getenv('TEMP') + "\\micro"

        self.library_folder = library_folder
        self.packages_folder = packages_folder
        self.build_folder = self.temp_folder
        self.distro = distro
        self.env = env

        self.dev_packages = []
        self.mcu_packages = []

        self.dev_folder = self.build_folder + '\\dev'
        self.dev_src_folder = self.dev_folder + '\\src'
        self.mcu_folder = self.build_folder + '\\mcu'
        self.mcu_src_folder = self.mcu_folder + '\\src'
        self.patch_folder = library_folder + '\\patchs\\' + self.distro

        self.library_path = library_folder + '\\libmicroros'
        self.library = self.library_path + "\\libmicroros.a"
        self.includes = self.library_path+ '\\include'
        self.library_name = "microros"

    def run(self, toolchain, user_meta = ""):
        if os.path.exists(self.library_path):
            print("micro-ROS already built")
            return

        # Delete previous build folders
        rmtree(self.temp_folder)
        os.makedirs(self.temp_folder)

        self.download_dev_environment()
        self.apply_patchs(self.dev_src_folder)
        self.build_dev_environment()
        self.download_mcu_environment()
        self.download_extra_packages()
        self.apply_patchs(self.mcu_src_folder)
        self.build_mcu_environment(toolchain, user_meta)
        self.package_mcu_library()

        # Delete generated build folders
        rmtree(self.temp_folder)

    def ignore_package(self, name):
        for p in self.mcu_packages:
            if p.name == name:
                p.ignore()

    def apply_patchs(self, target_folder):
        for patch_file in [x for x in os.listdir(self.patch_folder) if x.endswith('patch')]:
            repository_name = patch_file.split('.')[0]
            repository_path = target_folder + "\\" + repository_name

            # Check if repository exist
            if (os.path.isdir(repository_path)):

                # Apply patch
                patch_path = self.patch_folder + "\\" + patch_file
                command = "git apply {} --verbose".format(patch_path)
                result, stderr = run_cmd(command, env=self.env, capture_output=True, cwd=repository_path)

                if 0 != result:
                    print("{} repository patch failed: \n{}".format(repository_name, stderr))
                    sys.exit(1)

                print("\t - Patched {}".format(repository_name))

    def download_dev_environment(self):
        print("Downloading micro-ROS dev dependencies")
        for repo in Sources.dev_environments[self.distro]:
            repo.clone(self.dev_src_folder)
            print("\t - Downloaded {}".format(repo.name))
            self.dev_packages.extend(repo.get_packages())

    def build_dev_environment(self):
        print("Building micro-ROS dev dependencies")
        command = 'py -3 -m colcon build --packages-ignore-regex=.*_cpp --cmake-args -DBUILD_TESTING=OFF -G "Unix Makefiles"'.format()
        result, stderr = run_cmd(command, env=self.env, cwd=self.dev_folder)

        if 0 != result:
            print("Build dev micro-ROS environment failed\n")
            sys.exit(1)

    def download_mcu_environment(self):
        print("Downloading micro-ROS library")
        for repo in Sources.mcu_environments[self.distro]:
            repo.clone(self.mcu_src_folder)
            self.mcu_packages.extend(repo.get_packages())
            for package in repo.get_packages():
                if package.name in Sources.ignore_packages[self.distro] or package.name.endswith("_cpp"):
                    package.ignore()

                print('\t - Downloaded {}{}'.format(package.name, " (ignored)" if package.ignored else ""))

    def download_extra_packages(self):
        if not os.path.exists(self.packages_folder):
            print("\t - Extra packages folder not found, skipping...")
            return

        print("Checking extra packages")

        extra_folders = os.listdir(self.packages_folder)
    
        if '.gitkeep' in extra_folders:
            extra_folders.remove('.gitkeep')

        for folder in extra_folders:
            print("\t - Adding {}".format(folder))

        copy_tree(self.packages_folder, self.mcu_src_folder)

    def build_mcu_environment(self, toolchain_file, user_meta = ""):
        print("Building micro-ROS library")
        common_meta_path = self.library_folder + '\\metas\\common.meta'
        colcon_command = 'py -3 -m colcon build --merge-install --packages-ignore-regex=.*_cpp --metas {} {} --cmake-args -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=OFF -DTHIRDPARTY=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release --toolchain={} -G "Unix Makefiles"'.format(common_meta_path, user_meta, toolchain_file)
        command = "{}\\install\\setup.bat && {}".format(self.dev_folder, colcon_command)
        result, stderr = run_cmd(command, env=self.env, cwd=self.mcu_folder)

        if 0 != result:
            print("Build mcu micro-ROS environment failed\n")
            sys.exit(1)

    def package_mcu_library(self):
        aux_folder = self.build_folder + "\\temp"
        aux_naming_folder = aux_folder + "\\naming"

        os.makedirs(aux_folder)
        os.makedirs(aux_naming_folder)
        os.makedirs(self.library_path)

        AR = rtconfig.PREFIX + 'ar'
        # Generate object files with namespace prefix
        obj_list = []
        os.chdir(aux_naming_folder)
        for root, dirs, files in os.walk(self.mcu_folder + "\\install\\lib"):
            for f in files:
                if f.endswith('.a'):
                    os.system("{AR} x {PATH}".format(AR=AR, PATH=(root + "\\" + f)))
                    for obj in [x for x in os.listdir(aux_naming_folder) if x.endswith('obj')]:
                        updated_name = f.split('.')[0] + "__" + obj
                        os.rename(obj, '..\\' + updated_name)
                        obj_list.append(updated_name)

        # Create linker script
        os.chdir(aux_folder)
        with open("ar_script.m", "w+") as ar_script:
            ar_script.write("CREATE libmicroros.a\n")

            for element in obj_list:
                ar_script.write("ADDMOD {}\n".format(element))

            ar_script.write("SAVE\n")
            ar_script.write("END")

        # Execute linker script
        command = "{} -M < ar_script.m".format(AR)
        result, stderr = run_cmd(command, env=self.env)

        if 0 != result:
            print("micro-ROS static library build failed\n")
            sys.exit(1)

        os.rename('libmicroros.a', self.library)

        # Copy includes
        shutil.copytree(self.build_folder + "\\mcu\\install\\include", self.includes)

        # Fix include paths
        if self.distro not in ["galactic", "foxy"]:
            include_folders = os.listdir(self.includes)

            for folder in include_folders:
                folder_path = self.includes + "\\{}".format(folder)
                repeated_path = folder_path + "\\{}".format(folder)

                if os.path.exists(repeated_path):
                    copy_tree(repeated_path, folder_path)
                    rmtree(repeated_path)


class CMakeToolchain:
    def __init__(self, path, cc, cxx, ar, cflags, cxxflags):
        common_flags = " -DCLOCK_MONOTONIC=0 -DCLOCK_REALTIME=1 -D'__attribute__(x)='"
        cmake_toolchain = """
include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_CROSSCOMPILING 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_C_COMPILER {C_COMPILER})
set(CMAKE_CXX_COMPILER {CXX_COMPILER})
set(CMAKE_AR {AR_COMPILER})

set(CMAKE_C_FLAGS_INIT " {C_FLAGS} {COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT " {CXX_FLAGS} {COMMON_FLAGS} -fno-rtti" CACHE STRING "" FORCE)
set(__BIG_ENDIAN__ 0)"""

        cmake_toolchain = cmake_toolchain.format(C_COMPILER=cc, CXX_COMPILER=cxx, AR_COMPILER=ar, C_FLAGS=cflags, CXX_FLAGS=cxxflags, COMMON_FLAGS=common_flags)

        with open(path, "w") as file:
            file.write(cmake_toolchain)

        self.path = os.path.realpath(file.name)
