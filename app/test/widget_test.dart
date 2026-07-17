import 'package:flutter_test/flutter_test.dart';
import 'package:smart_pillbox/main.dart';

void main() {
  testWidgets('App launches and shows home title', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());
    await tester.pumpAndSettle();

    expect(find.text('智能药盒'), findsOneWidget);
  });
}
