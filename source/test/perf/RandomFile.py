import random, sys, re, os, hashlib

class RandomFile:
    def __init__(self, filename, size, object_size):
        self.filename = filename
        self.size = size
        self.object_size = object_size
        self.n_objects = int(size / object_size)

    def create_file(self):
        with open(self.filename, "wb") as f:
            f.write(os.urandom(self.size))

    def get_object(self):
        with open(self.filename, "rb") as f:
            data = f.read(self.object_size)
            while data:
                yield data
                data = f.read(self.object_size)

    def get_random_object(self):
        with open(self.filename, "rb") as f:
            idx = random.randrange(self.n_objects)
            f.seek(idx * self.object_size)
            yield f.read(self.object_size)

def test():
    rf = RandomFile("test", 1024*1024*1024, 4*1024)
    rf.create_file()
    for data in rf.get_random_object():
        if data != None:
            print hashlib.sha1(data).hexdigest()

if __name__ == "__main__":
    test()