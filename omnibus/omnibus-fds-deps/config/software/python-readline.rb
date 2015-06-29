name "python-readline"
default_version "6.2.4.1"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} readline==#{version}"
end
