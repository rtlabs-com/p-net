To build the documentation, it is important to use the sphinx-build script
installed in your virtual environment.
Use CMAKE_IGNORE_PATH to prevent cmake from using a script installed elsewhere

Run these commands (adapt to your needs):

python3 -m venv myvenv
source myvenv/bin/activate
pip3 install -r doc/requirements.txt

cmake -B build -DCMAKE_IGNORE_PATH:PATH=$HOME/.local/bin
cmake --build build/ --target sphinx-html
