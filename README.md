# 2023_RoboTracer
## 1. 概要

　この機体は，ロボトレース競技に参加するためにサークル標準機を改良したロボットです．ロボトレを始めてから1年が経過したため，今までの改良をまとめようと思います．また，改良内容の言語化による問題点の明確化やサークル内での情報共有も目的としています.

  
## 2. ディレクトリ構造

## 3. 開発環境
 - Visual Studio Code

 - Fusion360

 - Eagle

## 4. 機体説明

<p align="center">
 <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/6678fec6-45e2-4e66-b0d3-dd4fd7b67914" width="500px">
</p>

- 機体名 : Savoia S.21M


- スペック
  - L×W×H[mm] : 155x111x80
  - 重量[g] : 700
  - タイヤ径[mm] : 52
  - 最高速度[m/s] : 2.9
  - 最高加速度[m/s^2] : 4


- 構成要素
  - マイコンボード : Raspberry Pi Pico
  - アクチュエータ : PKE243A-L(ステッピングモータ)
  - モータドライバ　: SLA7078MPRT
  - バッテリ：LiPo/SIGP/3セル/1000mA
  - IMU : MPU-6050
  - ラインセンサ : LBR-127HLD
  - ADコンバータ : MCP3208

    
## 5. 改良点
- ラインセンサ

  　Raspberry Pi Picoには，ユーザが使えるADCが3つしかありません．そのため，12bitの外付けADコンバータによるADCの増設をしました．ラズピコの2チャンネル+ADコンバータの8チャンネルを使用し，計10個のラインセンサを使えるようにしました．
  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/81f3ebca-04ac-492f-b433-5287070fb427" width="500px">
  </p>



- モータref電圧の上昇

  　機体の速度を上げるとステッピングモータの脱調が起こるため、速度を上げる事が出来ませんでした。ステッピングモータは電圧をかけることでトルクと最高速度が上がるため，モータへかける電圧を上げました．電源電圧を上げる、またはref電圧を上げるといった方法が挙げられます。前者はコンデンサの交換等が必要となるため、改良が容易な後者を選択をしました．
  

- IMUの搭載
  
    　加減速走行をするにはロボットの角度を取得する必要があるため，ジャイロセンサを搭載しました．手持ちにあったMPU6050を使用しています．

  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/0492ced1-3b90-49bc-9984-d6cbd6b88eaa" width="500px">
  </p>
  
  
- CPU基盤の製作

   　標準機のCPU基盤はサイズが大きいため，ラインセンサとの干渉がありました．また，ADコンバータやジャイロセンサの搭載に対応させるため，CPU基盤を新たに製作しました．<p align="center">
　<img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/f866006d-cce5-4cd9-96a5-14833bb76c3b" width="500px">
</p>

- シャーシの製作

    　上記の改良に対応したシャーシを，3Dプリンタで製作しました．

  ![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/9e0da6db-21b0-4399-ba01-a1510e437fc8)

- タイヤ幅
  
  　標準機のタイヤ幅8mmは，重量700gに対して薄すぎたため，グリップ力が足りず180°のR10でスリップしていました．そのためタイヤ幅を厚くするパーツを製作しました．スポークに摩擦で固定しています．

## 6. ラインセンサのキャリブレーション
　　ラインセンサには様々な要因によって正確な値を取得することができません。ラインセンサを白線上に置き、横方向に1mmずつ移動させた時の各センサのアナログ値は以下のようになります。値は0-999としています

<p align="center">
 <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/7ecac7a3-7bff-4834-83ce-8b31b0fe02a6" width="500px">
</p>

結果から、
  - センサの個体差により黒色の時の値が異なる
  - 黒色でも値が高いためリニアな偏差を作る事が難しい
  - 白線時の値が９９９より小さい
    

    という問題が挙げられます。また、照明の影響で場所によって取得できる値が異なるため、センサ値のキャリブレーションを行う必要があります。

###    キャリブレーション方法

  白線では最大値、黒色は最小値となるため、この2値を用いて正規化を行います。正規化に用いる式を以下に示します。

$\[y=\frac{x-x_{min}}{x_{max}-x_{min}}\times 999\]$


キャリブレーション後の結果を以下に示します。黒色時に100以下、白線時に約999の値を取得する事ができるようになりました。


<p align="center">
 <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/e4ce5c64-d5fe-4997-b626-ee5b83c8bc5e" width="500px">
</p>
    

## 6. 制御方法
 　以前は、ラインセンサの値を2値化して制御していました。しかし、アナログ値を用いてより滑らかな走行にするためにPD制御を導入しました。

   ### PD制御

 制御について考えます。
PD制御を行うには、リニアな偏差eを求める必要があります．ラインセンサのアナログ値を左から L5，L4，L3，L2，L1，R1，R2...R5 とし，センサのアナログ値には距離が離れたセンサほど重みをつけるため，定数 k1，k2...k5 をかけます．
  
  <div align="center">
    <table><tr><td>
     <div align="center">
      $　e=(L5K5&plus;L4K4&plus;L3K3&plus;L2K2&plus;L1K1)-　$
     </div>  
     <div align="center">
      $　(R5K5&plus;R4K4&plus;R3K3&plus;R2K2&plus;R1K1)$
     </div>  
    </td></tr></table>
   </div>  
   
  　PD制御により制御量Controlを算出します．I成分は走行に違いが見られなかったため使用していません。
   
   <div align="center">
    <table><tr><td>
     $Control(t)=K_{P}e(t)&plus;K_{D}\dot{e}(t)$
    </td></tr></table>
   </div>
   
   　平均速度Vに制御量を与えて<img src="https://latex.codecogs.com/svg.image?\inline&space;V_{r}" title="V_{r}" />と<img src="https://latex.codecogs.com/svg.image?\inline&space;V_{l}" title="V_{l}" />を算出します。
    
  <div align="center">
   <table><tr><td>
    $V_{r}=V&plus;Control$
    <br>$V_{l}=V-Control$
   </td></tr></table>
  </div>
  
  
  <details><summary>PD制御プログラム</summary>
   
   ```Swift

dev = ( sen_val_c[9]*k5 + sen_val_c[7]*k4 + sen_val_c[6]*k3 + sen_val_c[5]*k2 + sen_val_c[4] ) - ( sen_val_c[8]*k5 + sen_val_c[0]*k4 + sen_val_c[1]*k3 + sen_val_c[2]*k2 + sen_val_c[3] );

control =  kp * dev  +  ( dev-dev_pre ) / 0.0004 * kd ;

target_spd_l = target_spd + control;
target_spd_r = target_spd - control;

dev_pre = dev;
```
</details>
<br>

## 7.ゴール判断

　ゴールセンサが反応した際にゴール判断をしてしまうと，クロスでもゴールセンサが反応してしまいます．そのため，クロスではゴール判断をしないという処理が必要となります．ゴールセンサがクロスに反応する前にラインセンサがクロスにかかるので，ライセンサを使ってクロスの判断ができます。両端のラインセンサが反応した時にフラグを立て，フラグが立っている時にゴールセンサが反応したら無視すればクロス問題は解決できそうです．

## 8.ジャイロセンサのフルスケールレンジ
  　走行中の角速度をグラフにすると，以下のようになります．
  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/624c51fc-5e0c-4b81-ba9d-a0cc6703ef79" width="800px">
  </p>
  
  　一定範囲外の値を正しく取得できていないことが分かります．これはジャイロのフルスケールレンジが関係しています．MPU6050の初期フルスケールレンジは±250deg/sとなっているため，±2000deg/sに設定します．
  
  <details><summary>フルスケールレンジ変更プログラム</summary>
 
  ```Swift
  
  #define MPU6050_ADDRESS 0x68
  
  uint8_t leg[] = {0x1B, 0x18};
  i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS,leg, sizeof(leg), false );
  
  ```
  
>  レジスタマップを参照してください。[0x1B]レジスタのbit4、bit3でジャイロのフルスケールレンジが設定できます。

</details>

  　改めてグラフを作ると，値を正しく取得できている事が分かります．

  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/64335dca-c771-4c69-a1c2-abb5698ec233" width="800px">
  </p>
  
## 8.ジャイロセンサを用いた角度の算出

 ロボットの角度は、角速度を積分する事で求められます。積分には様々な方法がありますが、計算での誤差を少なくするため、台形則を用います。面積を台形で近似するやつです。

<div align="center">
 <table><tr><td>
  <div align="center">
   $\theta _{t+1}=\theta _{t}+\left(\omega _{t+1}+\omega _{t}\right)\Delta t/2$
  </div>
  <br>
  <div align="center">
   $\Delta t\>$ : サンプリング周期[ms]，
   $\omega_{t}\>$ : t秒の角速度[deg]
  </div>
 </td></tr></table>
</div>

　上記の方法で角度の算出はできますが、オフセットなどによるドリフトが発生します。センサを1分間静止させた時の角度のずれを以下に示します．
 　
  <div align="center">
   
   | 回数 | 角度のずれ[deg/s] |
   | :---: | :---: |
   |1|-9.8005|
   |2|-9.8118|
   |3|-9.3929|
   |4|-8.8925|
   |5|-8.7566|
  
   </div>
  
  <br>
  
　平均すると約-9.3°のずれが生じることが分かりました。
 
### ジャイロセンサのキャリブレーション
 
 ドリフトを補正するため、センサのキャリブレーションを行います。静止時にオフセット値を取得し，オフセットを補正した状態でドリフト補正値を取得します。キャリブレーション後の式を以下に示します。(値の取得を初めてから数秒後の値を補正値として使用しています。最初の数秒は値が不安定になるようで、ドリフトが大きくなってしまいました。)
 
 <div align="center">
  <table><tr><td>
   <div align="center">
    $\theta _{t+1}=\theta _{t}+\left((\omega _{t+1}-offset+\omega _{t}\right)\Delta t/2)-drift$
   </div>
   <br>
   <div align="center">
    $offset$ : 静止時の平均角速度，
   </div>
   <div align="center">
    $drift$ : 一定時間ジャイロセンサを静止させ，
   </div>
   <div align="center">
    最後に取得した角速度をサンプリング周期当たりの補正値に変換したもの
   </div>
  </td></tr></table>
 </div>
 

  <details><summary>角度計算プログラム</summary>
   
   ```Swift
   #define GYRO_SENSITIVITY 16.4
   #define DELTA_T 0.003
   
   degree += ( ( ( (float)gyro[2]-gyro_offset + degree_pre ) / GYRO_SENSITIVITY ) * DELTA_T / 2 ) - gyro_drift;
   degree_pre = (float)gyro[2] - gyro_offset;
   ```
>センサからの値を物理量に変換するために，レンジに対応した分解能で割ります．今回は±2000deg/sで使用するため、分解能は16.4LSB/deg/sとします。
</details>

キャリブレーション後の角度のずれを以下の表に示します。
  <div align="center">
   
   | 回数 | calibなし | calibあり |
   | :---: | :---: | :---: |
   |1|-9.8005|0.0085|
   |2|-9.8118|0.0129|
   |3|-9.3929|-0.0665|
   |4|-8.8925|-0.0134|
   |5|-8.7566|0.0320|

   
   </div>
1分間で平均-0.0053°までずれを抑えることができました．

   
## 9.マッピング

　加減速走行を行うには，再現性のある走行とコースの記憶が必要となります．そのため走行経路を2次元座標にプロットし，再現性の確認とコースの記憶をできるようにしました．加減速走行は区間距離と角速度情報があれば出来るため、コースのプロットをする必要がありません．しかし，まだ先の話ですがショートカット走行を行う際に役立つと考えたので開発を行いました。
 
### 座標の算出方法
 
 コースに固定された座標系を定義します。また、トレーサの運動を対向二輪ロボットの運動として考えます。

  Δt秒間でのロボットの移動を以下のように仮定します。

  <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/fd787d87-81b7-4480-9cf6-d06ddfdb928a" width="400px">
  </p>


  ロボットを取り除くと、以下の図が得られます。

 <p align="center">
   <img src="https://github.com/MasA-23/2023_RoboTracer/assets/147514546/159e9625-ccb6-48e5-bf4b-d1598f85df05" width="400px">
  </p>
 
ステッピングモータを使用しているため、まずは1ステップ当たりに進む距離kを求めます。

 <div align="center">
  <table><tr><td>
   <div align="center">
   $k=\theta _{s}/360\cdot 2\pi r$
    </div>
   <div align="center">
    $\theta _{s}$ : ステップ角，
    $r$ : 車輪半径
    </div>
  </td></tr></table>
 </div>


$\Delta t$で進む距離
$\Delta l$を求めます。Δtは微小区間のためΔlは直線と考えます。左右車輪の走行距離の平均を取ることで、車体中央の走行距離を求めます。

<div align="center">
  <table><tr><td>
   <div align="center">
$\Delta l=(\Delta s_{l}+\Delta s_{r})k/2$
    </div>
   <div align="center">
    $s_{r}$ : 左車輪のステップ数，
    $s_{l}$ : 右車輪のステップ数，
    </div>
</td></tr></table>
 </div>
 
次に$t$秒におけるロボットの角度
$\theta _{t}\$を求めます。

<div align="center">
  <table><tr><td>
    $\theta _{t+1}=\theta _{t}+\left((\omega _{t+1}-offset+\omega _{t}\right)\Delta t/2)-drift$
  </td></tr></table>
</div>

 ジャイロを使わず、左右のステップ数から角度を求める事もできます。
 
 <div align="center">
  <table><tr><td>
    $\theta _{t+1}=\theta _{t}+\Delta\theta $
    $\Delta\theta=tan^{-1}((\Delta s_{r}-\Delta s_{l})k/2d)$
  </td></tr></table>
 </div>

 
最後に、ロボットの座標
$(x_{t+1},y_{t+1})$を算出します。xtにytにそれぞれの軸成分を積算します。


<div align="center">
  <table><tr><td>
   <div align="center">
    $x_{t+1}=x_{t}+\Delta lsin\theta_{t}$
    </div>
   <div align="center">
$y_{t+1}=y_{t}+\Delta lcos\theta _{t}$
</div>
</td></tr></table>
 </div>

### マッピング結果

以下に二次元座標にプロットされたコースを示します。

![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/e645200a-2967-442e-8bc1-62915128776b)

5回走行させた時の目標座標とのズレを以下の表に示します。

![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/77ae9640-e018-4b99-9a6c-e49c791723cd)

実験結果から、平均してmmのずれがある事が分かります。



## 10.加減速走行

　直線を台形加速させる、最も初歩的なアルゴリズムであると思われます。区間距離と角速度を用いて加減速を行います。

 ### 速度計画の流れ

   1. 区間距離を求める
   2. 直線か曲線か区別
   3. 直線且つ最高速度まで加速可能な距離がある場合、加減速距離を求める
   4. 直線且つ最高速度まで加速可能な距離がない場合、区間で出せる最高速度を求める
   5. 曲線は開発中
  
 - 詳細
   1. 区間距離の算出
      マッピングで求めたΔlを用いて走行距離ltを求める。マーカ反応時の距離の差分を取れば区間距離が求まる

   2.直線と曲線の区別
   
   区間の最高角速度を参照し、直線と曲線を区別しようと考えていました。しかし、マーカセンサよりラインセンサが前方に配置されているため、マーカセンサが反応するタイミングではすでにラインセンサが次の区間の制御を行なっています。そのため、今の区間が直線でも次の区間が曲線の場合、今の区間が曲線と区別されてしまいます。そのため、区間中央で取得した角速度を用いる事で、区間の区別の精度を上げました。

   3. 直線且つ最高速度まで加速可能な距離がある場合、加減速距離を求める
  
      

## 11.角速度のノイズ除去

第40回北陸信越地区大会の試走会において、直線で加速しない問題が発生しました。ログを見てみると、直線で取得した角速度が大きいため、曲線として認識されていました。

大会で取得した角速度を以下に示します。

![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/82ac20b9-bd51-490d-b0b3-9939d2231b6c)


大学のコースで取得した以下の角速度グラフと比較すると、大会コースデータにはノイズが多く含まれる事がわかります。

![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/9161650f-319f-407a-a321-32a759c77ed3)


原因について詳細には分かりませんが、大学より会場の光量が強かったため、制御が発振してしまったと考えられます。そのため、移動平均法を用いたノイズの除去を行いました。平均範囲は150msです。ノイズ除去後の角速度データを以下に示します。

![image](https://github.com/MasA-23/2023_RoboTracer/assets/147514546/465a1659-a133-4d22-8897-93845f5d3485)


ノイズを除去する事ができました。

[参考にさせていただいたサイト](https://github.com/raspberrypi/pico-examples/blob/master/i2c/mpu6050_i2c/mpu6050_i2c.c)

[参考にさせていただいたサイト](https://qiita.com/jamjam/items/6d49f9200d809b4a1d72)
