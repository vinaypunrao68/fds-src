name "fstab"
default_version "1.4"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} #{name}==#{version}"
end
