from subprocess import PIPE, Popen
import os
import stat
import json

def run_cmd(command, env=None, capture_output=False, cwd=None):

	if (capture_output):
		p = Popen(command, shell=True, env=env, stdout=PIPE, stderr=PIPE, cwd=cwd)
	else:
		p = Popen(command, shell=True, env=env, cwd=cwd)

	stdout, stderr = p.communicate()
	return p.returncode, stderr

def rmtree(directory):
    if (os.path.isdir(directory)):
        for root, dirs, files in os.walk(directory, topdown=False):
            for name in files:
                filename = os.path.join(root, name)
                os.chmod(filename, stat.S_IWUSR)
                os.remove(filename)
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        os.rmdir(directory)

class EnvironmentHandler:
	def __init__(self):
		self.modified_env = os.environ.copy()

	def get_env(self):
		return self.modified_env

	def set_environment_variable(self, variable, value):
		self.modified_env[variable] = value

	def reset_environment(self):
		self.modified_env = os.environ.copy()

	def find_and_set_python3(self):
		path = self.modified_env['PATH'].split(";")
		possible_python_path = [x for x in path if "Python3" in x and "Scripts" not in x]

		if len(possible_python_path) >= 1:
			python_script_path = [x for x in path if "Python3" in x and "Scripts" in x]
			path.insert(0, possible_python_path[0])
			path.insert(0, python_script_path[0])
			self.set_environment_variable('PATH', ";".join(path))
			self.set_environment_variable('PYTHONHOME', ";".join(possible_python_path))
			self.set_environment_variable('PYTHONPATH', ";".join(possible_python_path))

			# Check that pip is installed
			run_cmd('py -3 -m ensurepip', env=self.modified_env, capture_output=True)

			return True
		else:
			return False

	def install_python_dependencies(self, deps):
		# Install dependencies
		pip_command = Popen('py -3 -m pip freeze', shell=True, env=self.modified_env, stdout=PIPE, stderr=PIPE)
		stdout, stderr = pip_command.communicate()
		pip_packages = [x.split("==")[0] for x in stdout.split('\n')]
		required_packages = deps
		to_install = []
		for req in required_packages:
			if req.split('==')[0].lower() not in [y.lower() for y in pip_packages]:
				to_install.append(req)

		if len(to_install) == 0:
			print("All required Python pip packages are installed")

		for p in to_install:
			print('Installing {} with pip at RT-Thread environment'.format(p))
			run_cmd('py -3 -m pip install {}'.format(p), env=self.modified_env, capture_output=False)

class MetaFileGenerator:
	def __init__(self, path):
		self.meta = {"names": {}}
		self.path = path
		self.save()

	def set_variable(self, package, var, value):
		if package not in self.meta["names"]:
			self.meta["names"][package] = {"cmake-args": []}
		self.meta["names"][package]["cmake-args"].append("-D" + var +  "=" + str(value))
		self.save()

	def save(self):
		with open(self.path, "w") as file:
			file.write(json.dumps(self.meta, indent=4))