name "requests"
default_version "2.6.0"

dependency "pip"
dependency "urllib3"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} #{name}==#{version}"
end
