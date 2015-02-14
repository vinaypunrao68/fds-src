name "ansible"
default_version "1.8.2"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} #{name}==#{version}"
end
