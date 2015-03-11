name "pycurl"
default_version "7.19.5"

dependency "pip"
dependency "curl"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} #{name}==#{version}"
end
