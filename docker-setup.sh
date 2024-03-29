# Script to install needed stuff to compile metamod addon

# docker pull registry.gitlab.steamos.cloud/steamrt/scout/sdk
# docker run -v D:\repos:/dockerVolume -it <imageID>

export HL2SDKCS2='/dockerVolume/cs2-prac/sdk'
export MMSOURCE_DEV='/dockerVolume/metamod-source'
export CC=clang
export CXX=clang++

echo --------------------------------------------------------------------------
echo Starting steam sniper compilator for metamod
echo

apt-get update
apt-get install python3-setuptools -y
apt-get install clang -y
apt-get install python3 -y
apt-get install gcc -y

cd "/dockerVolume/ambuild"
python3 setup.py install

cd "/dockerVolume/cs2-prac"
mkdir build
cd build

python3 ../configure.py --sdks cs2 --targets x86_64
ambuild


# Copy files to the deploy folder
todaysDate=$(date +%d-%b-%H_%M)
#mkdir ../../dockerVolume/$todaysDate
#DEPLOY_FOLDER=../../dockerVolume/$todaysDate

echo finished
echo $todaysDate