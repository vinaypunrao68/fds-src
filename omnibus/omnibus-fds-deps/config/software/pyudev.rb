name "pyudev"
default_version "0.17"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} #{name}==#{version}"
end
