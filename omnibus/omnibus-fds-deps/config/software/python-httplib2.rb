name "python-httplib2"
default_version "0.9.1"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} httplib2==#{version}"
end
