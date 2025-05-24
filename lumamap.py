import sys

CC_MODE = '--cc' in sys.argv

sectionRowLengths = [2, 5, 6, 6, 6, 6, 6, 6, 6, 5, 2]
sectionMap = []
keyNo = 0
rowNo = 0
xoffs = 0

for rowLen in sectionRowLengths:
    for n in range(rowLen):
        sectionMap.append({
            'key': keyNo + n,
            'x': n + xoffs,
            'y': rowNo
        })
    rowNo += 1
    if rowNo in [2,4,6,8]:
        xoffs -= 1
    if rowNo == 9:
        xoffs += 1
    if rowNo == 10:
        xoffs += 2

    keyNo += rowLen

def coordToColor(board, x, y):
    x += 5*board - 5*2
    y += 2*board - 2*2 - 5
    key31 = (5 * x + 3 * y + 8) % 31
    if key31 in [0,5,8,13,18,21,26]:
        return 'DFDFDF'
    elif key31 in [2,7,10,15,20,23,28]:
        return '176991'
    elif key31 in [29,3,6,11,16,19,24]:
        return '916C17'
    elif key31 in [12,25,30]:
        return '783FA7'
    elif key31 in [14,27,1]:
        return 'A7453C'
    elif key31 in [4,9,17,22]:
        return 'B35E96'
    else:
        return '000000'
    
    

for board in range(5):
    print(f'[Board{board}]')
    for key in sectionMap:
        kn = key['key']
        print(f'Key_{kn}={kn}')
        print(f'Chan_{kn}={board+2}')
        print(f'Col_{kn}={coordToColor(board, key["x"], key["y"])}')
        print(f'CCInvert_{kn}')
        if CC_MODE:
            print(f'KTyp_{kn}=2')


print('AfterTouchActive=1')
print('LightOnKeyStrokes=0')
print('InvertFootController=0')
print('InvertSustain=1')
print('ExprCtrlSensitivity=0')
print('VelocityIntrvlTbl=1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 60 61 62 63 64 66 67 68 70 71 72 73 74 76 77 79 81 82 84 86 88 90 92 94 96 98 101 104 107 111 115 119 124 129 134 140 146 152 159 170 171 175 180 185 190 195 200 205 210 215 220 225 230 235 240 245 250 255 260 265 270 275 280 285 290 295 300 305 310')
print(f'NoteOnOffVelocityCrvTbl={" ".join(map(str,range(128)))}')
print(f'FaderConfig={" ".join(map(str,range(128)))}')
print(f'afterTouchConfig={" ".join(map(str,range(128)))}')
print(f'LumaTouchConfig={" ".join(map(str,range(128)))}')
