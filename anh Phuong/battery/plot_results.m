a = load('D:\Phuong\NCS 2016\Phuonghv\TapLTC++\battery\battery\ketqua.txt');

%subplot(511)
%plot(a(:,1), a(:,2))
%legend('hsnapxa');
%legend('show');
%grid

subplot(411)
plot(a(:,1), a(:,3),'r')
ylabel('Qt(Ah)');
legend('Qt(Ah)-C++');
legend('show');
grid

subplot(412)
plot(a(:,1), a(:,4)*100,'r')
hold
plot(tout, out(:,1))
ylabel('SOC (%)');
legend('C++', 'Matlab');
legend('show');
grid

subplot(413)
plot(a(:,1), a(:,5),'r')
hold
plot(tout, out(:,3))
ylabel('Ubatt (V)');
legend('C++', 'Matlab');
legend('show');
grid


subplot(414)
plot(a(:,1), a(:,6),'r')
hold
plot(tout, out(:,2))
ylabel('Ibatt (A)');
legend('C++', 'Matlab');
legend('show');
xlabel('Time (s)');
grid