import config
import ConfigParser
import logging
import os
 
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)
    
def parse(fname):
    params = {}
    parser = ConfigParser.ConfigParser()
    config_file = os.path.join(config.CONFIG_DIR, fname)
    parser.read(config_file)
    sections = parser.sections()
    for section in sections:
        options = parser.options(section)
        for option in options:
            try:
                params[option] = parser.get(section, option)
                if params[option] == -1:
                    log.info("skipping: %s" % option)
            except:
                log.info("Exception on %s!" % option)
                params[option] = None
    return params