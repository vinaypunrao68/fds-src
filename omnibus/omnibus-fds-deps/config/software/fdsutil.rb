name "fdsutil"
default_version "0.3.5"

dependency "pip"
dependency "python-readline"

build do
  command "#{install_dir}/embedded/bin/pip install --extra-index-url=https://builder:h7qgA3fYr0P3@artifacts.artifactoryonline.com/artifacts/api/pypi/formation-pip/simple -I --build #{project_dir} fdsutil==#{version}"
end
